/*
	main.c
*/

#include "main.h"

PSP_MODULE_INFO( "MacroFire", PSP_MODULE_KERNEL, 0, 0 );

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static void mf_init           ( void );
static void mf_loadstore_ini  ( const char *inipath );
static void mf_call_intr      ( const int mfengine );
static void mf_shutdown       ( void );
static int  mf_read_pad_buffer( MfSceCtrlDataFunc func, SceCtrlData *pad, int count, MfCallMode mode );

static enum mf_interrupt_handler_mode mf_interrupt_mode( void );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static bool         st_apihooked = false;
static MfRunEnv     st_runenv    = false;
static SceCtrlData  st_pad_data;
static unsigned int st_prev_pad_buttons;

/*=============================================*/

static void mf_init( void )
{
	int i;
	
	/* メニューを初期化 */
	mfMenuInit();
	
	/* 実行環境をチェック */
	st_runenv = MF_RUNENV_GAME;
	for( i = 0; i < 5; i++ ){
		if( sceKernelFindModuleByName( "sceVshBridge_Driver" ) != NULL ){
			st_runenv = MF_RUNENV_VSH;
			break;
		} else if( sceKernelFindModuleByName( "scePops_Manager" ) != NULL ){
			st_runenv = MF_RUNENV_POPS;
			break;
		}
		sceKernelDelayThread( 200000 );
	}
	
	/* フックアドレスを取得 */
	for( i = 0; i < MF_ARRAY_NUM( hook ); i++ ){
		hook[i].restore.addr = hookFindSyscallAddr( hookFindExportAddr( hook[i].module, hook[i].library, hook[i].nid ) );
		if( hook[i].restore.addr ){
			hook[i].restore.entrypoint = *(hook[i].restore.addr);
		}
	}
	
	/* 初期化関数呼び出し */
	for( i = 0; i < mftableEntry; i++ ){
		if( mftable[i].initFunc ) ( mftable[i].initFunc )();
	}
}

static void mf_loadstore_ini( const char *inipath )
{
	IniUID ini;
	
	ini = inimgrNew();
	if( ini > 0 ){
		int i;
		char buttons[128];
		if( inimgrLoad( ini, MF_INI_FILENAME ) != 0 ){
			inimgrSetBool( ini, "Main", "Startup", MF_INIDEF_MAIN_STARTUP );
			inimgrSetString( ini, "Main", "MenuButtons",   MF_INIDEF_MAIN_MENUBUTTONS );
			inimgrSetString( ini, "Main", "ToggleButtons", MF_INIDEF_MAIN_TOGGLEBUTTONS );
			
			analogtuneCreateIni( ini );
			
			for( i = 0; i < mftableEntry; i++ ){
				if( mftable[i].ciFunc ) ( mftable[i].ciFunc )( ini );
			}
			
			inimgrSave( ini, MF_INI_FILENAME );
		}
		
		gMfEngine = inimgrGetBool( ini, "Main", "Startup", false );
		
		inimgrGetString( ini, "Main", "MenuButtons", MF_INIDEF_MAIN_MENUBUTTONS, buttons, sizeof( buttons ) );
		gMfMenu = ctrlpadUtilStringToButtons( buttons );
		
		inimgrGetString( ini, "Main", "ToggleButtons", MF_INIDEF_MAIN_TOGGLEBUTTONS, buttons, sizeof( buttons ) );
		gMfToggle = ctrlpadUtilStringToButtons( buttons );
		
		analogtuneLoadIni( ini );
		
		for( i = 0; i < mftableEntry; i++ ){
			if( mftable[i].liFunc ) ( mftable[i].liFunc )( ini );
		}
		inimgrDestroy( ini );
	}
}

static void mf_call_intr( const int mfengine )
{
	int i;
	for( i = 0; i < mftableEntry; i++ ){
		if( mftable[i].hook.intr ) ( mftable[i].hook.intr )( mfengine );
	}
}

static void mf_shutdown( void )
{
	int i;
	
	/* メニュー終了処理 */
	//mfMenuTerm();
	
	if( st_apihooked ) mfRestoreApi();
	
	/* 終了関数呼び出し */
	for( i = 0; i < mftableEntry; i++ ){
		if( mftable[i].termFunc ) ( mftable[i].termFunc )();
	}
}

static enum mf_interrupt_handler_mode mf_interrupt_mode( void )
{
	/*
		例外ハンドラを呼び出した時の実行モードを得る。
		ちゃんと取得できているか不明。
	*/
	uint32_t cop0_status_register;
	
	/* ステータスレジスタを読み出す */
	asm( "mfc0 %0, $12;" : "=r"( cop0_status_register ) );
	
	/* ステータスレジスタの2ビット目が現在の例外ハンドラの実行モード */
	return ( cop0_status_register & 0x00000002 ) ? MF_IHM_KERNEL : MF_IHM_USER;
}

static int mf_read_pad_buffer( MfSceCtrlDataFunc func, SceCtrlData *pad, int count, MfCallMode mode )
{
	/*
		K1レジスタのメモ
		mf_read_pad_buffer()は内部で第1引数に指定された本来のAPIを実行する。
		しかし本来のコードは、ファームウェア内部のOSカーネル専用領域に存在する。
		ここにアクセスするには、特権モードに移行しなければならない。
		
		普通のAPIはシステムコール例外の例外ハンドラとして動いているようで、
		APIをコールするとSYSCALL命令が発行され、例外ハンドラを実行する際に
		特権モードへ移行するという仕掛けを使って実行できるようになっているっぽい？
		
		pspSdkSetK1( 0 )を実行すると特権モードに移行させられるようだ。
		これを行ったあとならばファームウェア内のコードを直接実行できる。
		
		やることやったら元の値に戻せるように、最初のpspSdkSetK1()の返り値を保存しておく。
	*/
	
	unsigned int k1 = pspSdkSetK1( 0 );
	enum mf_interrupt_handler_mode ihm;
	int ret;
	
	ret = ( func )( pad, count );
	ihm = mf_interrupt_mode();

	pspSdkSetK1( k1 );
	
	analogtuneTune( pad, NULL );
	mfRapidfire( 0, mode, pad );
	
	if( mode != MF_CALL_INTERNAL ){
		int i;
		for( i = 0; i < mftableEntry; i++ ){
			if( mftable[i].hook.func ) ( mftable[i].hook.func )( mode, pad, mftable[i].hook.arg );
		}
		if( ihm == MF_IHM_USER ) pad->Buttons &= 0x0000FFFF;
	}
	
	return ret;
}

int mfVshCtrlReadBufferPositive( SceCtrlData *pad, int count )
{
	return mf_read_pad_buffer( hook[MF_VSHCTRL_READ_BUFFER_POSITIVE].restore.entrypoint, pad, count, MF_CALL_READ );
}

int mfCtrlPeekBufferPositive( SceCtrlData *pad, int count )
{
	return mf_read_pad_buffer( hook[MF_CTRL_PEEK_BUFFER_POSITIVE].restore.entrypoint, pad, count, MF_CALL_READ );
}

int mfCtrlPeekBufferNegative( SceCtrlData *pad, int count )
{
	int ret = mf_read_pad_buffer( hook[MF_CTRL_PEEK_BUFFER_POSITIVE].restore.entrypoint, pad, count, MF_CALL_READ );
	
	pad->Buttons = ~pad->Buttons;
	
	return ret;
}

int mfCtrlReadBufferPositive( SceCtrlData *pad, int count )
{
	return mf_read_pad_buffer( hook[MF_CTRL_READ_BUFFER_POSITIVE].restore.entrypoint, pad, count, MF_CALL_READ );
}

int mfCtrlReadBufferNegative( SceCtrlData *pad, int count )
{
	int ret = mf_read_pad_buffer( hook[MF_CTRL_READ_BUFFER_POSITIVE].restore.entrypoint, pad, count, MF_CALL_READ );
	
	pad->Buttons = ~pad->Buttons;
	
	return ret;
}

int mfCtrlPeekLatch( SceCtrlLatch *latch )
{
	SceCtrlData pad;
	int ret;
	unsigned int im;
	
	im = pspSdkDisableInterrupts();
	ret = mf_read_pad_buffer( hook[MF_CTRL_PEEK_BUFFER_POSITIVE].restore.entrypoint, &pad, 1, MF_CALL_LATCH );
	pspSdkEnableInterrupts( im );
	
	latch->uiMake    |= ( st_prev_pad_buttons ^ pad.Buttons ) & pad.Buttons;
	latch->uiBreak   |= ( st_prev_pad_buttons ^ pad.Buttons ) & st_prev_pad_buttons;
	latch->uiPress   |= pad.Buttons;
	latch->uiRelease = ~0;
	
	st_prev_pad_buttons = pad.Buttons;
	
	return ret;
}

int mfCtrlReadLatch( SceCtrlLatch *latch )
{
	SceCtrlData pad;
	int ret;
	unsigned int im;
	
	im = pspSdkDisableInterrupts();
	ret = mf_read_pad_buffer( hook[MF_CTRL_PEEK_BUFFER_POSITIVE].restore.entrypoint, &pad, 1, MF_CALL_LATCH );
	pspSdkEnableInterrupts( im );
	
	latch->uiMake    = ( st_prev_pad_buttons ^ pad.Buttons ) & pad.Buttons;
	latch->uiBreak   = ( st_prev_pad_buttons ^ pad.Buttons ) & st_prev_pad_buttons;
	latch->uiPress   = pad.Buttons;
	latch->uiRelease = ~pad.Buttons;
	
	st_prev_pad_buttons = pad.Buttons;
	
	return ret;
}

void mfHookApi( void )
{
	int i;
	
	if( st_apihooked ) return;
	
	for( i = 0; i < MF_ARRAY_NUM( hook ); i++ ){
		hookFunc( hook[i].restore.addr, hook[i].entrypoint );
	}
	
	st_apihooked = true;
}

void mfRestoreApi( void )
{
	int i;
	
	if( ! st_apihooked ) return;
	
	for( i = 0; i < MF_ARRAY_NUM( hook ); i++ ){
		hookFunc( hook[i].restore.addr, hook[i].restore.entrypoint );
	}
	
	st_apihooked = false;
}

int main_thread( SceSize arglen, void *argp )
{
	bool wait_toggle_button_release = false;
	
	/* 初期化 */
	mf_init();
	
	/* INI読み込み */
	mf_loadstore_ini( MF_INI_FILENAME );
	
	while( gRunning ){
		/* module_stop()が呼ばれないので、一時停止をして強制終了可能な状態にする */
		sceKernelDelayThread( 50000 );
		
		if( st_apihooked ){
			mf_read_pad_buffer( hook[MF_CTRL_PEEK_BUFFER_POSITIVE].restore.entrypoint, &st_pad_data, 1, MF_CALL_INTERNAL );
		} else{
			sceCtrlPeekBufferPositive( &st_pad_data, 1 );
			analogtuneTune( &st_pad_data, NULL );
			st_pad_data.Buttons |= ctrlpadUtilGetAnalogDirection( st_pad_data.Lx, st_pad_data.Ly, 0 );
		}
		
		st_pad_data.Buttons =  mfButtonsUmask( st_pad_data.Buttons, MF_UNUSED_BUTTONS );
		st_pad_data.Buttons |= ctrlpadUtilGetAnalogDirection( st_pad_data.Lx, st_pad_data.Ly, 0 );
		
		if( gMfToggle ){
			if( ( st_pad_data.Buttons & (~( ~0 ^ gMfToggle )) ) == gMfToggle ){
				if( ! wait_toggle_button_release ){
					gMfEngine = ( gMfEngine ? false : true );
					wait_toggle_button_release = true;
				}
			} else if( wait_toggle_button_release ){
				wait_toggle_button_release = false;
			}
		}
		
		if( ! st_apihooked && gMfEngine ){
			/* APIをフック */
			mfHookApi();
			mf_call_intr( gMfEngine );
		} else if( st_apihooked && ! gMfEngine ){
			/* APIをリストア */
			mfRestoreApi();
			mf_call_intr( gMfEngine );
		}
		
		if( ( st_pad_data.Buttons & (~( ~0 ^ gMfMenu )) ) == gMfMenu ) mfMenu();
	}
	
	/* module_stop()が呼ばれないので、以下は実行されていない */
	
	mf_shutdown();
	
	return sceKernelExitDeleteThread( 0 );
}

bool mfIsApiHooked( void )
{
	return st_apihooked;
}

void mfEnable( void )
{
	gMfEngine = true;
}

void mfDisable( void )
{
	gMfEngine = false;
}

bool mfIsEnabled( void )
{
	return gMfEngine;
}

bool mfIsDisabled( void )
{
	return ( ! gMfEngine );
}

MfRunEnv mfRunEnv( void )
{
	return st_runenv;
}

unsigned int mfButtonsMask( unsigned int buttons, unsigned int mask )
{
	return buttons & mask;
}

unsigned int mfButtonsUmask( unsigned int buttons, unsigned int umask )
{
	return buttons & ( 0xFFFFFFFF ^ umask );
}


int module_start( SceSize arglen, void *argp )
{
	SceUID thid;
	
	thid = sceKernelCreateThread( "MacroFire", main_thread, 32, 0x1200, 0, 0 );
	if( thid ) sceKernelStartThread( thid, arglen, argp );
	
	return 0;
}

int module_stop( SceSize arglen, void *argp )
{
	/* 呼び出されていない。どうすればいいのか？ */
	
	gRunning = false;
	return 0;
}

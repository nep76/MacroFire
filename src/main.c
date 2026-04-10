/*
	main.c
*/

#include "main.h"

PSP_MODULE_INFO( "MacroFire", PSP_MODULE_KERNEL, 0, 0 );

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static void mf_find_hookaddr  ( void );
static void mf_call_intr      ( const int mfengine );
static int  mf_read_pad_buffer( MfSceCtrlDataFunc func, SceCtrlData *pad, int count, MfCallMode mode );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static bool         st_apihooked = false;
static MfRunEnv     st_runenv    = false;
static SceCtrlData  st_pad_data;
static unsigned int st_prev_pad_buttons;

/*=============================================*/

static void mf_detect_runenv( void )
{
	/* 実行環境をチェック */
	int i;
	
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
}

static void mf_find_hookaddr( void )
{
	int i;
	
	for( i = 0; i < MF_ARRAY_NUM( Hooktable ); i++ ){
		Hooktable[i].origfunc = (void *)sctrlHENFindFunction( Hooktable[i].modname, Hooktable[i].library, Hooktable[i].nid );
	}
}

static void mf_call_intr( const int mfengine )
{
	int i;
	for( i = 0; i < mftableEntry; i++ ){
		if( mftable[i].hook.intr ) ( mftable[i].hook.intr )( mfengine );
	}
}

static int mf_read_pad_buffer( MfSceCtrlDataFunc func, SceCtrlData *pad, int count, MfCallMode mode )
{
	/*
		K1レジスタのメモ
		mf_read_pad_buffer()は内部で第1引数に指定された本来のAPIを実行する。
		しかし本来のコードは、ファームウェア内部のOSカーネル専用領域に存在する。
		ここにアクセスするには、特権モードに移行しなければならない。
		
		普通にAPIはシステムコール例外の例外ハンドラとして動いているようで、
		APIをコールするとSYSCALL命令が発行され、例外ハンドラを実行する際に
		特権モードへ移行するという仕掛けを使って実行できるようになっているっぽい？
		
		pspSdkSetK1( 0 )を実行すると特権モードに移行させられるようだ。
		つまり、これを行ったあとならばファームウェア内のコードを直接実行できる。
		
		やることやったら元の値に戻せるように、最初のpspSdkSetK1()の返り値を保存しておく。
	*/
	int ret = ( func )( pad, count );
	
	analogtuneTune( pad, NULL );
	mfRapidfire( 0, mode, pad );
	
	if( mode != MF_CALL_INTERNAL ){
		int i;
		for( i = 0; i < mftableEntry; i++ ){
			if( mftable[i].hook.func ) ( mftable[i].hook.func )( mode, pad, mftable[i].hook.arg );
		}
	}
	return ret;
}

int mfVshCtrlReadBufferPositive( SceCtrlData *pad, int count )
{
	return mf_read_pad_buffer( Hooktable[MF_VSHCTRL_READ_BUFFER_POSITIVE].origfunc, pad, count, MF_CALL_READ );
}

int mfCtrlPeekBufferPositive( SceCtrlData *pad, int count )
{
	return mf_read_pad_buffer( Hooktable[MF_CTRL_PEEK_BUFFER_POSITIVE].origfunc, pad, count, MF_CALL_READ );
}

int mfCtrlPeekBufferNegative( SceCtrlData *pad, int count )
{
	int ret = mf_read_pad_buffer( Hooktable[MF_CTRL_PEEK_BUFFER_POSITIVE].origfunc, pad, count, MF_CALL_READ );
	
	pad->Buttons = ~pad->Buttons;
	
	return ret;
}

int mfCtrlReadBufferPositive( SceCtrlData *pad, int count )
{
	return mf_read_pad_buffer( Hooktable[MF_CTRL_READ_BUFFER_POSITIVE].origfunc, pad, count, MF_CALL_READ );
}

int mfCtrlReadBufferNegative( SceCtrlData *pad, int count )
{
	int ret = mf_read_pad_buffer( Hooktable[MF_CTRL_READ_BUFFER_POSITIVE].origfunc, pad, count, MF_CALL_READ );
	
	pad->Buttons = ~pad->Buttons;
	
	return ret;
}

int mfCtrlPeekLatch( SceCtrlLatch *latch )
{
	SceCtrlData pad;
	int ret;
	unsigned int k1 = pspSdkSetK1( 0 );
	
	ret = mf_read_pad_buffer( Hooktable[MF_CTRL_PEEK_BUFFER_POSITIVE].origfunc, &pad, 1, MF_CALL_LATCH );
	
	pspSdkSetK1( k1 );
	
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
	unsigned int k1 = pspSdkSetK1( 0 );
	
	ret = mf_read_pad_buffer( Hooktable[MF_CTRL_PEEK_BUFFER_POSITIVE].origfunc, &pad, 1, MF_CALL_LATCH );
	
	pspSdkSetK1( k1 );
	
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
	
	for( i = 0; i < MF_ARRAY_NUM( Hooktable ); i++ ){
		if( ! Hooktable[i].origfunc || ! Hooktable[i].hookfunc ) continue;
		sctrlHENPatchSyscall( (u32)Hooktable[i].origfunc, Hooktable[i].hookfunc );
	}
	
	st_apihooked = true;
}

void mfRestoreApi( void )
{
	int i;
	
	if( ! st_apihooked ) return;
	
	for( i = 0; i < MF_ARRAY_NUM( Hooktable ); i++ ){
		if( ! Hooktable[i].origfunc || ! Hooktable[i].hookfunc ) continue;
		sctrlHENPatchSyscall( (u32)Hooktable[i].hookfunc, Hooktable[i].origfunc );
	}
	
	st_apihooked = false;
}

int main_thread( SceSize arglen, void *argp )
{
	bool wait_toggle_button_release = false;
	
	/* メニューを初期化 */
	mfMenuInit();
	
	/* 実行環境を取得 */
	mf_detect_runenv();
	
	/* フックアドレスを取得 */
	mf_find_hookaddr();
	
	{
		IniUID ini;
		int i;
		
		/* 初期化関数呼び出し */
		for( i = 0; i < mftableEntry; i++ ){
			if( mftable[i].initFunc ) ( mftable[i].initFunc )();
		}
		
		ini = inimgrNew();
		if( ini > 0 ){
			char buttons[128];
			if( inimgrLoad( ini, MFM_INI_FILENAME ) != 0 ){
				inimgrSetBool( ini, "Main", "Startup", MF_INIDEF_MAIN_STARTUP );
				inimgrSetString( ini, "Main", "MenuButtons",   MF_INIDEF_MAIN_MENUBUTTONS );
				inimgrSetString( ini, "Main", "ToggleButtons", MF_INIDEF_MAIN_TOGGLEBUTTONS );
				
				analogtuneCreateIni( ini );
				
				for( i = 0; i < mftableEntry; i++ ){
					if( mftable[i].ciFunc ) ( mftable[i].ciFunc )( ini );
				}
				
				inimgrSave( ini, MFM_INI_FILENAME );
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
	
	while( gRunning ){
		/* module_stop()が呼ばれないので、一時停止をして強制終了可能な状態にする */
		sceKernelDelayThread( 50000 );
		
		if( st_apihooked ){
			unsigned int k1 = pspSdkSetK1( 0 );
			mf_read_pad_buffer( Hooktable[MF_CTRL_PEEK_BUFFER_POSITIVE].origfunc, &st_pad_data, 1, MF_CALL_INTERNAL );
			pspSdkSetK1( k1 );
		} else{
			sceCtrlPeekBufferPositive( &st_pad_data, 1 );
			analogtuneTune( &st_pad_data, NULL );
			st_pad_data.Buttons |= ctrlpadUtilGetAnalogDirection( st_pad_data.Lx, st_pad_data.Ly, 0 );
		}
		
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
	
	/* メニュー終了処理 */
	//mfMenuTerm();
	
	if( st_apihooked ) mfRestoreApi();
	
	{
		/* 終了関数呼び出し */
		int i;
		for( i = 0; i < mftableEntry; i++ ){
			if( mftable[i].termFunc ) ( mftable[i].termFunc )();
		}
	}
	
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

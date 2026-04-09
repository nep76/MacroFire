/*
	MacroFire
*/

/* フックはM33SDKのsctrlHENFindFunction(),sctrlHENPatchSyscall()を使ったほうがいいのだろうか？ */

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
static SceCtrlData  st_pad_data;
static unsigned int st_prev_pad_buttons;

/*=============================================*/

static void mf_find_hookaddr( void )
{
	int i;
	
	for( i = 0; i < MF_ARRAY_NUM( Hooktable ); i++ ){
		Hooktable[i].exported.entrypoint = hookFindExportedAddr(
			Hooktable[i].modname,
			Hooktable[i].library,
			Hooktable[i].nid
		);
		Hooktable[i].exported.addr = hookFindSyscallAddr( Hooktable[i].exported.entrypoint );
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
	pad->Buttons |= ctrlpadUtilGetAnalogDirection( pad->Lx, pad->Ly, 0 );
	
	mfRapidfire( 0, mode, pad );
	
	if( mode != MF_CALL_INTERNAL ){
		int i;
		for( i = 0; i < mftableEntry; i++ ){
			if( mftable[i].hook.func ) ( mftable[i].hook.func )( mode, pad, mftable[i].hook.arg );
		}
	}
	return ret;
}

int mfCtrlPeekBufferPositive( SceCtrlData *pad, int count )
{
	return mf_read_pad_buffer( Hooktable[0].exported.entrypoint, pad, count, MF_CALL_READ );
}

int mfCtrlPeekBufferNegative( SceCtrlData *pad, int count )
{
	int ret = mf_read_pad_buffer( Hooktable[0].exported.entrypoint, pad, count, MF_CALL_READ );
	
	pad->Buttons = ~pad->Buttons;
	
	return ret;
}

int mfCtrlReadBufferPositive( SceCtrlData *pad, int count )
{
	return mf_read_pad_buffer( Hooktable[2].exported.entrypoint, pad, count, MF_CALL_READ );
}

int mfCtrlReadBufferNegative( SceCtrlData *pad, int count )
{
	int ret = mf_read_pad_buffer( Hooktable[2].exported.entrypoint, pad, count, MF_CALL_READ );
	
	pad->Buttons = ~pad->Buttons;
	
	return ret;
}

int mfCtrlPeekLatch( SceCtrlLatch *latch )
{
	SceCtrlData pad;
	int ret;
	unsigned int k1 = pspSdkSetK1( 0 );
	
	ret = mf_read_pad_buffer( Hooktable[0].exported.entrypoint, &pad, 1, MF_CALL_LATCH );
	
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
	
	ret = mf_read_pad_buffer( Hooktable[0].exported.entrypoint, &pad, 1, MF_CALL_LATCH );
	
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
		if( ! Hooktable[i].exported.addr || ! Hooktable[i].hookfunc ) continue;
		hookUpdateExportedAddr( Hooktable[i].exported.addr, Hooktable[i].hookfunc );
	}
	
	st_apihooked = true;
}

void mfRestoreApi( void )
{
	int i;
	
	if( ! st_apihooked ) return;
	
	for( i = 0; i < MF_ARRAY_NUM( Hooktable ); i++ ){
		if( ! Hooktable[i].exported.addr || ! Hooktable[i].exported.entrypoint ) continue;
		hookUpdateExportedAddr( Hooktable[i].exported.addr, Hooktable[i].exported.entrypoint );
	}
	
	st_apihooked = false;
}

int main_thread( SceSize arglen, void *argp )
{
	bool wait_toggle_button_release = false;
	
	/* メニューを初期化 */
	mfMenuInit();
	
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
			if( inimgrLoad( ini, "ms0:/seplugins/macrofire.ini" ) != 0 ){
				inimgrSetBool( ini, "Main", "Startup", MF_INIDEF_MAIN_STARTUP );
				inimgrSetString( ini, "Main", "MenuButtons",   MF_INIDEF_MAIN_MENUBUTTONSNAME );
				inimgrSetString( ini, "Main", "ToggleButtons", MF_INIDEF_MAIN_TOGGLEBUTTONSNAME );
				
				analogtuneCreateIni( ini );
				
				for( i = 0; i < mftableEntry; i++ ){
					if( mftable[i].ciFunc ) ( mftable[i].ciFunc )( ini );
				}
				
				inimgrSave( ini, "ms0:/seplugins/macrofire.ini" );
			}
			
			gMfEngine = inimgrGetBool( ini, "Main", "Startup", false );
			
			inimgrGetString( ini, "Main", "MenuButtons", MF_INIDEF_MAIN_MENUBUTTONSNAME, buttons, sizeof( buttons ) );
			gMfMenu = ctrlpadUtilStringToButtons( buttons );
			
			inimgrGetString( ini, "Main", "ToggleButtons", MF_INIDEF_MAIN_TOGGLEBUTTONSNAME, buttons, sizeof( buttons ) );
			gMfToggle = ctrlpadUtilStringToButtons( buttons );
			
			analogtuneLoadIni( ini );
			
			for( i = 0; i < mftableEntry; i++ ){
				if( mftable[i].liFunc ) ( mftable[i].liFunc )( ini );
			}
			inimgrDestroy( ini );
		}
	}
	
	/* 起動を待つ */
	sceKernelDelayThread( 3000000 );
	
	while( gRunning ){
		/* module_stop()が呼ばれないので、一時停止をして強制終了可能な状態にする */
		sceKernelDelayThread( 50000 );
		
		if( st_apihooked ){
			unsigned int k1 = pspSdkSetK1( 0 );
			mf_read_pad_buffer( Hooktable[0].exported.entrypoint, &st_pad_data, 1, MF_CALL_INTERNAL );
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

int module_start( SceSize arglen, void *argp )
{
	SceUID thid;
	
	thid = sceKernelCreateThread( "MacroFire", main_thread, 32, 0x1200, 0, 0 );
	
	/* MacroFireへカレントディレクトリ設定 */
	if( thid ){
		sceIoChangeThreadCwd( thid, MFM_WORKING_DIRECTORY );
		sceKernelStartThread( thid, arglen, argp );
	}
	
	return 0;
}

int module_stop( SceSize arglen, void *argp )
{
	/* 呼び出されていない。どうすればいいのか？ */
	
	gRunning = false;
	return 0;
}

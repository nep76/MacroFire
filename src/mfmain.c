/*==========================================================

	mfmain.c

	MacroFire メインループ

==========================================================*/
#include "mfmain.h"
#include "psp/ovmsg.h"

PSP_MODULE_INFO( "MacroFire", PSP_MODULE_KERNEL, 3, 0 );

/*=========================================================
	ローカル関数
=========================================================*/
static void mf_init( void );
static void mf_shutdown( void );
static void mf_ini( const char *inipath );
static void mf_ini_load( IniUID ini );
static void mf_ini_save( IniUID ini );
static int mf_get_ctrl_buffer( MfHookAction action, unsigned int api, SceCtrlData *pad, MfHprmKey *hk );
static void mf_ctrl_data_copy( SceCtrlData *padarr, int count );

/*=========================================================
	ローカル変数
=========================================================*/
static char st_game_id[11];
static char *st_ini_request_section;
static char *st_ini_target_section;

static unsigned int st_world        = 0;
static unsigned int st_prev_buttons = 0;
static u32          st_prev_hprmkey = 0;

static bool           st_hooked          = false;
static bool           st_hook_incomplete = false;

static bool           st_is_enabled     = MF_INI_STARTUP;
static PadutilButtons st_menu_buttons   = MF_INI_MENU_BUTTONS;
static PadutilButtons st_toggle_buttons = MF_INI_TOGGLE_BUTTONS;

static PadutilButtonName *st_cvbtn;

static SceCtrlData st_pad;
static MfHprmKey   st_hk;

/*-----------------------------------------------
	MacroFireの初期化と終了
-----------------------------------------------*/
static void mf_init( void )
{
	SceUID umd;
	int i;
	MfFuncInit initfunc;
	
	/* 最初に行う */
	dbgprint( "Initializing menu" );
	if( ! mfMenuInit() ) gRunning = false;
	
	/*
		実行環境を取得
		合計で10秒程度待つ。
	*/
	dbgprint( "Detecting world..." );
	st_world = MF_WORLD_GAME;
	for( i = 0; i < 5; i++ ){
		if( sceKernelFindModuleByName( "sceVshBridge_Driver" ) != NULL ){
			st_world = MF_WORLD_VSH;
			break;
		} else if( sceKernelFindModuleByName( "scePops_Manager" ) != NULL ){
			st_world = MF_WORLD_POPS;
			break;
#ifdef DEBUG_ENABLED
		} else if( sceKernelFindModuleByName( "PSPLINK" ) != NULL ){
			break;
#endif
		}
		dbgprint( "Waiting for loading modules..." );
		sceKernelDelayThread( 2000000 );
	}
	
	/* フックアドレスを取得 */
	dbgprint( "Find hook address..." );
	for( i = 0; i < hooktableEntryCount; i++ ){
		hooktable[i].restore.addr = hookFindSyscallAddr( hookFindExportAddr( hooktable[i].module, hooktable[i].library, hooktable[i].nid ) );
		dbgprintf( "\tNID:0x%08x Addr:%p Func:%p", hooktable[i].nid, hooktable[i].restore.addr, hooktable[i].hook );
		if( hooktable[i].restore.addr ){
			hooktable[i].restore.api = *(hooktable[i].restore.addr);
		} else if( st_world == MF_WORLD_VSH ){
			if( i == MF_VSHCTRL_READ_BUFFER_POSITIVE ) st_hook_incomplete = true;
		} else if( 
			i == MF_CTRL_PEEK_BUFFER_POSITIVE || i == MF_CTRL_PEEK_BUFFER_NEGATIVE ||
			i == MF_CTRL_READ_BUFFER_POSITIVE || i == MF_CTRL_READ_BUFFER_NEGATIVE
		){
			st_hook_incomplete = true;
		}
	}
	
	/* ゲームIDを取得 */
	umd = sceIoOpen( "disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0777 ); 
	if( umd > 0 ){
		sceIoRead( umd, st_game_id, 10 );
		sceIoClose( umd );
	}
	
	/* ファンクションの初期化 */
	dbgprint( "Sending message MF_MS_INIT to all functions..." );
	for( i = 0; i < gMftabEntryCount; i++ ){
		if( gMftab[i].proc ){
			initfunc = ( gMftab[i].proc )( MF_MS_INIT );
			if( initfunc ) initfunc();
		}
	}
}

static void mf_shutdown( void )
{
	int i;
	MfFuncTerm termfunc;
	
	mfUnhook();
	
	/* ファンクションの終了 */
	dbgprint( "Sending message MF_MS_TERM to all functions..." );
	for( i = 0; i < gMftabEntryCount; i++ ){
		if( gMftab[i].proc ){
			termfunc = ( gMftab[i].proc )( MF_MS_TERM );
			if( termfunc ) termfunc();
		}
	}
	
	if( mfOverlayMessageIsRunning() ) mfOverlayMessageExit();
	
	mfMenuDestroy();
}

/*-----------------------------------------------
	INIの読み込みと保存
-----------------------------------------------*/
static void mf_ini( const char *inipath )
{
	IniUID ini = inimgrNew();
	
	if( ini < 0 ){
		dbgprintf( "Failed to create IniUID: %x", ini );
		return;
	} else{
		if( mfConvertButtonReady() ){
			if( inimgrLoad( ini, inipath, NULL, 0, 0, 0 ) == 0 ){
				dbgprintf( "%s found", inipath );
				mf_ini_load( ini );
			} else{
				dbgprintf( "%s not found", inipath );
				mf_ini_save( ini );
				inimgrSave( ini, inipath, NULL, 0, 0, 0 );
			}
			mfConvertButtonFinish();
		}
		inimgrDestroy( ini );
	}
}

static void mf_ini_load( IniUID ini )
{
	int i;
	char buf[255];
	MfFuncIni inifunc;
	
	bool status_notification = MF_INI_STATUS_NOTIFICATION;
	
	/* Mainセクション読み出し */
	inimgrGetBool( ini, "Main", "Startup", &st_is_enabled );
	if( inimgrGetString( ini, "Main", "MenuButtons", buf, sizeof( buf ) ) > 0 )   st_menu_buttons = mfConvertButtonN2C( buf );
	if( inimgrGetString( ini, "Main", "ToggleButtons", buf, sizeof( buf ) ) > 0 ) st_toggle_buttons = mfConvertButtonN2C( buf );
	inimgrGetBool( ini, "Main", "StatusNotification", &status_notification );
	
	/* 設定ファイルのロードすべきセクション名を決定 */
	if( st_game_id[0] == '\0' || ! inimgrExistSection( ini, st_game_id ) ){
		switch( mfGetWorld() ){
			case MF_WORLD_VSH:
				st_ini_target_section = MF_INI_SECTION_VSH;
				break;
			case MF_WORLD_POPS:
				st_ini_target_section = MF_INI_SECTION_POPS;
				break;
			case MF_WORLD_GAME:
				st_ini_target_section = MF_INI_SECTION_GAME;
				break;
		}
		st_ini_request_section = st_game_id[0] == '\0' ? st_ini_target_section : st_game_id;
		if( ! inimgrExistSection( ini, st_ini_target_section ) ) st_ini_target_section = MF_INI_SECTION_DEFAULT;
	} else{
		st_ini_request_section = st_game_id;
		st_ini_target_section  = st_game_id;
	}
	
	/* セクション名を確定したので、各機能へロード要求 */
	mfMenuIniLoad( ini, buf, sizeof( buf ) );
	mfAnalogStickIniLoad( ini, buf, sizeof( buf ) );
	
	dbgprint( "Sending message MF_MS_INI_LOAD to all functions..." );
	for( i = 0; i < gMftabEntryCount; i++ ){
		if( gMftab[i].proc ){
			inifunc = ( gMftab[i].proc )( MF_MS_INI_LOAD );
			if( inifunc ) inifunc( ini, buf, sizeof( buf ) );
		}
	}
	
	if( status_notification ) mfOverlayMessageStart();
}

static void mf_ini_save( IniUID ini )
{
	int i;
	char buf[255];
	MfFuncIni inifunc;
	
	inimgrSetBool( ini, "Main", "Startup", st_is_enabled );
	
	mfConvertButtonC2N( st_menu_buttons, buf, sizeof( buf ) );
	inimgrSetString( ini, "Main", "MenuButtons", buf );
	
	mfConvertButtonC2N( st_toggle_buttons, buf, sizeof( buf ) );
	inimgrSetString( ini, "Main", "ToggleButtons", buf );
	
	inimgrSetBool( ini, "Main", "StatusNotification", MF_INI_STATUS_NOTIFICATION );
	
	mfMenuIniSave( ini, buf, sizeof( buf ) );
	mfAnalogStickIniSave( ini, buf, sizeof( buf ) );
	
	dbgprint( "Sending message MF_MS_INI_CREATE to all functions..." );
	for( i = 0; i < gMftabEntryCount; i++ ){
		if( gMftab[i].proc ){
			inifunc = ( gMftab[i].proc )( MF_MS_INI_CREATE );
			if( inifunc ) inifunc( ini, buf, sizeof( buf ) );
		}
	}
}

/*-----------------------------------------------
	指定されたAPIでパッドデータを取得し、そのデータをファンクションへ渡す。
	
	K1レジスタのメモ
		この関数は第2引数に指定された本来のAPIを実行する。
		しかし本来のコードは、ファームウェア内部のOSカーネル専用領域に存在する。
		ここにアクセスするには、特権モードに移行しなければならない。
		
		普通のAPIはシステムコール例外の例外ハンドラとして動いているようで、
		APIをコールするとSYSCALL命令が発行され、例外ハンドラを実行する際に
		特権モードへ移行するという仕掛けを使って実行できるようになっているっぽい？
		
		pspSdkSetK1( 0 )を実行すると特権モードに移行させられるようだ。
		これを行ったあとならばファームウェア内のコードを直接実行できる。
		
		やることやったら元の値に戻せるように、最初のpspSdkSetK1()の返り値を保存しておく。
-----------------------------------------------*/
static int mf_get_ctrl_buffer( MfHookAction action, unsigned int api, SceCtrlData *pad, MfHprmKey *hk )
{
	unsigned int k1_register;
	int ret;
	MfFuncHook hookfunc;

	k1_register = pspSdkSetK1( 0 );
	
	if( hooktable[api].restore.api ){
		ret = ( ( int (*)( SceCtrlData*, int ) )hooktable[api].restore.api )( pad, 1 );
	} else{
		switch( api ){
			case MF_CTRL_PEEK_BUFFER_POSITIVE:    ret = sceCtrlPeekBufferPositive( pad, 1 ); break;
			case MF_CTRL_PEEK_BUFFER_NEGATIVE:    ret = sceCtrlPeekBufferNegative( pad, 1 ); break;
			case MF_CTRL_READ_BUFFER_POSITIVE:    ret = sceCtrlReadBufferPositive( pad, 1 ); break;
			case MF_CTRL_READ_BUFFER_NEGATIVE:    ret = sceCtrlReadBufferNegative( pad, 1 ); break;
			case MF_VSHCTRL_READ_BUFFER_POSITIVE: ret = sceCtrlReadBufferPositive( pad, 1 ); break;
			default: return CG_ERROR_INVALID_ARGUMENT;
		}
	}
	if( hooktable[MF_HPRM_PEEK_CURRENT_KEY].restore.api ){
		( ( int (*)( u32* ) )hooktable[MF_HPRM_PEEK_CURRENT_KEY].restore.api )( hk );
	} else{
		*hk = 0;
	}
	
	pspSdkSetK1( k1_register );
	
	/* アナログスティック調整 */
	hookfunc = mfAnalogStickProc( MF_MS_HOOK );
	if( hookfunc ) hookfunc( action, pad, hk );
	
	mfRapidfireReset();
	
	if( action != MF_INTERNAL ){
		int i;
		for( i = 0; i < gMftabEntryCount; i++ ){
			if( gMftab[i].proc ){
				hookfunc = ( gMftab[i].proc )( MF_MS_HOOK );
				if( hookfunc ) hookfunc( action, pad, hk );
			}
		}
		//if( caller == MF_CALLER_USER ) pad->Buttons &= MF_TARGET_BUTTONS;
	}
	
	return ret;
}

static void mf_ctrl_data_copy( SceCtrlData *padarr, int count )
{
	int i;
	
	for( i = 1; i < count; i++ ){
		padarr[i] = padarr[0];
	}
}

/*-----------------------------------------------
	MF_MS_TOGGLEメッセージを全てのファンクションに送る
-----------------------------------------------*/
static void mf_sending_toggle_message( bool engine )
{
	int i;
	MfFuncToggle togglefunc;
	
	if( mfOverlayMessageIsRunning() ){
		mfOverlayMessagePrintf( "MacroFire Engine: %s", engine ? "ON" : "OFF" );
	}
	
	dbgprint( "Sending message MF_MS_TOGGLE to all functions..." );
	for( i = 0; i < gMftabEntryCount; i++ ){
		if( gMftab[i].proc ){
			togglefunc = ( gMftab[i].proc )( MF_MS_TOGGLE );
			if( togglefunc ) togglefunc( engine );
		}
	}
}

/*-----------------------------------------------
	MacroFire メインループ
-----------------------------------------------*/
static int mf_main( SceSize arglen, void *argp )
{
	bool wait_toggle_buttons_release = false;
	
	dbgprint( "Initializing" );
	mf_init();
	
	dbgprint( "Checking ini" );
	{
		char *inipath = memoryAlloc( arglen + strlen( MF_INI_FILENAME ) );
		if( inipath ){
			char *dirpath = strrchr( (const char *)argp, '/' );
			if( dirpath ){
				strncpy( inipath, (const char *)argp, dirpath - (const char *)argp + 1 );
				strcat( inipath, MF_INI_FILENAME );
				mf_ini( inipath );
			}
			memoryFree( inipath );
		}
	}
	
	dbgprint( "Starting main-loop" );
	while( gRunning ){
		sceKernelDelayThread( 50000 );
		
		mf_get_ctrl_buffer( MF_INTERNAL, MF_CTRL_PEEK_BUFFER_POSITIVE, &st_pad, &st_hk );
		st_pad.Buttons |= padutilGetAnalogStickDirection( st_pad.Lx, st_pad.Ly, 0 );
		
		if( st_toggle_buttons ){
			if( ( st_pad.Buttons & st_toggle_buttons ) == st_toggle_buttons ){
				if( ! wait_toggle_buttons_release ){
					st_is_enabled = ( st_is_enabled ? false : true );
					wait_toggle_buttons_release = true;
					dbgprintf( "Changing engine state: %d", st_is_enabled );
				}
			} else if( wait_toggle_buttons_release ){
				wait_toggle_buttons_release = false;
			}
		}
		
		if( ! st_hooked && st_is_enabled ){
			mfHook();
			mf_sending_toggle_message( st_is_enabled );
		} else if( st_hooked && ! st_is_enabled ){
			mfUnhook();
			mf_sending_toggle_message( st_is_enabled );
		}
		
		if( ( ( padutilSetPad( st_pad.Buttons ) | padutilSetHprm( st_hk ) ) & st_menu_buttons ) == st_menu_buttons ){
			dbgprint( "Launching menu" );
			mfMenuMain( &st_pad, &st_hk );
		}
	}
	/* module_stop()が呼ばれないので、以下は実行されていない */
	
	dbgprint( "Shutting down..." );
	mf_shutdown();
	
	dbgprint( "Exit" );
	return sceKernelExitDeleteThread( 0 );
}

/*-----------------------------------------------
	APIのフック
	
	既にフックされている状態でもう一度呼んでもなにもしない。
-----------------------------------------------*/
void mfHook( void )
{
	int i;
	
	if( st_hooked ) return;
	
	dbgprint( "Hooking API" );
	
	st_prev_buttons = 0;
	
	for( i = 0; i < hooktableEntryCount; i++ ){
		dbgprintf( "Hooking %p -> %p", hooktable[i].restore.addr, hooktable[i].hook );
		hookFunc( hooktable[i].restore.addr, hooktable[i].hook );
		dbgprint( "OK" );
	}
	
	st_hooked = true;
}

/*-----------------------------------------------
	フックの解除
	
	まだフックされていない状態で呼んでも何もしない。
-----------------------------------------------*/
void mfUnhook( void )
{
	int i;
	
	if( ! st_hooked ) return;
	
	dbgprint( "Unhooking API" );
	
	for( i = 0; i < hooktableEntryCount; i++ ){
		dbgprintf( "Unhooking %p -> %p", hooktable[i].hook, hooktable[i].restore.addr );
		hookFunc( hooktable[i].restore.addr, hooktable[i].restore.api );
		dbgprint( "OK" );
	}
	
	st_hooked = false;
}

/*-----------------------------------------------
	フック関数
-----------------------------------------------*/
int mfVshctrlReadBufferPositive( SceCtrlData *pad, int count )
{
	int ret;
	MfHprmKey hk;
	
	ret = mf_get_ctrl_buffer( MF_UPDATE, MF_VSHCTRL_READ_BUFFER_POSITIVE, pad, &hk );
	if( count > 1 ) mf_ctrl_data_copy( pad, count );
	
	return ret;
}

int mfCtrlReadBufferPositive( SceCtrlData *pad, int count )
{
	int ret;
	MfHprmKey hk;
	
	ret = mf_get_ctrl_buffer( MF_UPDATE, MF_CTRL_READ_BUFFER_POSITIVE, pad, &hk );
	if( count > 1 ) mf_ctrl_data_copy( pad, count );
	
	return ret;
}

int mfCtrlReadBufferNegative( SceCtrlData *pad, int count )
{
	int ret;
	MfHprmKey hk;
	
	ret = mf_get_ctrl_buffer( MF_UPDATE, MF_CTRL_READ_BUFFER_POSITIVE, pad, &hk );
	pad->Buttons = ~pad->Buttons;
	if( count > 1 ) mf_ctrl_data_copy( pad, count );
	
	return ret;
}

int mfCtrlPeekBufferPositive( SceCtrlData *pad, int count )
{
	int ret;
	MfHprmKey hk;
	
	ret = mf_get_ctrl_buffer( MF_UPDATE, MF_CTRL_PEEK_BUFFER_POSITIVE, pad, &hk );
	if( count > 1 ) mf_ctrl_data_copy( pad, count );
	
	return ret;
}

int mfCtrlPeekBufferNegative( SceCtrlData *pad, int count )
{
	int ret;
	MfHprmKey hk;
	
	ret = mf_get_ctrl_buffer( MF_UPDATE, MF_CTRL_PEEK_BUFFER_POSITIVE, pad, &hk );
	pad->Buttons = ~pad->Buttons;
	if( count > 1 ) mf_ctrl_data_copy( pad, count );
	
	return ret;
}

int mfCtrlPeekLatch( SceCtrlLatch *latch )
{
	SceCtrlData pad;
	MfHprmKey hk;
	int ret;
	unsigned int intc;
	
	intc = pspSdkDisableInterrupts();
	
	ret = mf_get_ctrl_buffer( MF_KEEP, MF_CTRL_PEEK_BUFFER_POSITIVE, &pad, &hk );
	
	latch->uiMake    |= ( st_prev_buttons ^pad.Buttons ) & pad.Buttons;
	latch->uiBreak   |= ( st_prev_buttons ^ pad.Buttons ) & st_prev_buttons;
	latch->uiPress   |= pad.Buttons;
	latch->uiRelease = ~0;
	
	st_prev_buttons = pad.Buttons;
	
	pspSdkEnableInterrupts( intc );
	
	return ret;
}

int mfCtrlReadLatch( SceCtrlLatch *latch )
{
	SceCtrlData pad;
	MfHprmKey hk;
	int ret;
	unsigned int intc;
	
	intc = pspSdkDisableInterrupts();
	
	ret = mf_get_ctrl_buffer( MF_KEEP, MF_CTRL_PEEK_BUFFER_POSITIVE, &pad, &hk );
	
	latch->uiMake    = ( st_prev_buttons ^ pad.Buttons ) & pad.Buttons;
	latch->uiBreak   = ( st_prev_buttons ^ pad.Buttons ) & st_prev_buttons;
	latch->uiPress   = pad.Buttons;
	latch->uiRelease = ~pad.Buttons;
	
	st_prev_buttons = pad.Buttons;
	
	pspSdkEnableInterrupts( intc );
	
	return ret;
}

int mfHprmPeekCurrentKey( u32 *key )
{
	SceCtrlData pad;
	return mf_get_ctrl_buffer( MF_UPDATE, MF_CTRL_PEEK_BUFFER_POSITIVE, &pad, key );
}

int mfHprmPeekLatch( u32 *latch )
{
	SceCtrlData pad;
	MfHprmKey hk;
	int ret;
	unsigned int intc;
	
	intc = pspSdkDisableInterrupts();
	
	ret = mf_get_ctrl_buffer( MF_KEEP, MF_CTRL_PEEK_BUFFER_POSITIVE, &pad, &hk );
	
	*latch |= ( st_prev_hprmkey ^ hk ) & hk;
	st_prev_hprmkey = hk;
	
	pspSdkEnableInterrupts( intc );
	
	return ret;
}

int mfHprmReadLatch( u32 *latch )
{
	/* mfHprmPeekLatch()との違いがよく分からない */
	return mfHprmPeekLatch( latch );
}

/*-----------------------------------------------
	メインAPI
-----------------------------------------------*/
void mfEnable( void )
{
	st_is_enabled = true;
}

void mfDisable( void )
{
	st_is_enabled = false;
}

bool mfIsEnabled( void )
{
	return st_is_enabled;
}

void mfSetMenuButtons( PadutilButtons buttons )
{
	st_menu_buttons = buttons;
}

void mfSetToggleButtons( PadutilButtons buttons )
{
	st_toggle_buttons = buttons;
}

PadutilButtons mfGetMenuButtons( void )
{
	return st_menu_buttons;
}

PadutilButtons mfGetToggleButtons( void )
{
	return st_toggle_buttons;
}

bool mfConvertButtonReady( void )
{
	if( ! st_cvbtn ){
		st_cvbtn = padutilCreateButtonNames();
		if( ! st_cvbtn ) return false;
	}
	return true;
}

void mfConvertButtonFinish( void )
{
	if( st_cvbtn ){
		padutilDestroyButtonList( st_cvbtn );
		st_cvbtn = NULL;
	}
}

char *mfConvertButtonC2N( PadutilButtons buttons, char *buf, size_t len )
{
	padutilGetButtonNamesByCode( st_cvbtn, buttons, "+", PADUTIL_OPT_TOKEN_SP, buf, len );
	return buf;
}

PadutilButtons mfConvertButtonN2C( char *buttons )
{
	PadutilButtons btncode;
	btncode = padutilGetButtonCodeByNames( st_cvbtn, buttons, "+", PADUTIL_OPT_IGNORE_SP );
	return btncode;
}

const char *mfGetGameId()
{
	return st_game_id;
}

const char *mfGetIniRequestSection( void )
{
	return st_ini_request_section;
}

const char *mfGetIniTargetSection( void )
{
	return st_ini_target_section;
}

unsigned int mfGetWorld( void )
{
	return st_world;
}

bool mfIsRunningApp( MfAppId app )
{
	switch( app ){
		case MF_APP_WEBBROWSER:
			if( sceKernelFindModuleByName( "sceHVNetfront_Module" ) != NULL ) return true;
			break;
		default:
			break;
	}
	return false;
}

int mfOverlayMessageStart( void )
{
	SceUID msg_thid;
	
	ovmsgInit();
	
	msg_thid = sceKernelCreateThread( "MacroFireOverlayMessage", ovmsgThreadMain, 16, 0x200, 0, 0 );
	if( msg_thid > 0 ){
		sceKernelStartThread( msg_thid, 0, NULL );
		ovmsgWaitForReady();
		return 0;
	} else{
		return msg_thid;
	}
}

void mfOverlayMessageExit( void )
{
	ovmsgShutdownStart();
}

bool mfOverlayMessageIsRunning( void )
{
	return ovmsgIsRunning();
}

bool mfOverlayMessagePrintf( const char *format, ... )
{
	va_list ap;
	bool ret;
	
	va_start( ap, format );
	
	ovmsgPrintIntrStart();
	ovmsgWaitForReady();
	ret = ovmsgVprintf( format, ap );
	
	va_end( ap );
	
	return ret;
}

bool mfHookIncomplete( void )
{
	return st_hook_incomplete;
}

HeapUID mfHeapCreate( unsigned int count, size_t size )
{
	return heapCreateEx( "MacroFireHeap", MEMORY_USER, 0, size + HEAP_HEADER_SIZE * count, PSP_SMEM_High, NULL );
}

/*-----------------------------------------------
	エントリポイント
-----------------------------------------------*/
#ifdef MF_WITH_EXCEPTION_HANDLER
static const unsigned char regName[32][5] =
{
    "zr", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", 
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

static const char *codeTxt[32] =
{
	"Interrupt", "TLB modification", "TLB load/inst fetch", "TLB store",
	"Address load/inst fetch", "Address store", "Bus error (instr)",
	"Bus error (data)", "Syscall", "Breakpoint", "Reserved instruction",
	"Coprocessor unusable", "Arithmetic overflow", "Unknown 13", "Unknown 14",
	"FPU Exception", "Unknown 16", "Unknown 17", "Unknown 18",
	"Unknown 20", "Unknown 21", "Unknown 22", "Unknown 23",
	"Unknown 24", "Unknown 25", "Unknown 26", "Unknown 27",
	"Unknown 28", "Unknown 29", "Unknown 30", "Unknown 31"
};

void mf_exception( PspDebugRegBlock *regs )
{
	SceUID fd;
	int i;
	
	fd = sceIoOpen( "ms0:/macrofire_error.txt", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	if( fd ){
		char buf[256];
		SceModule *mod;
		SceKernelModuleInfo modinfo;
		unsigned int baseaddr = 0;
		
		mod = sceKernelFindModuleByAddress( regs->epc );
		if( mod )
		{
			memset( &modinfo, 0, sizeof( SceKernelModuleInfo ) );
			modinfo.size = sizeof( SceKernelModuleInfo );
			
			if( sceKernelQueryModuleInfo( mod->modid, &modinfo ) == 0 ){
				baseaddr = modinfo.text_addr;
			}
		}
		
		snprintf( buf, sizeof( buf ), "MacroFire Version %s\n", MF_VERSION );
		sceIoWrite( fd, buf, strlen( buf ) );
		snprintf( buf, sizeof( buf ), "Exception - %s\n", codeTxt[(regs->cause >> 2) & 31] );
		sceIoWrite( fd, buf, strlen( buf ) );
		snprintf( buf, sizeof( buf ), "EPC       - %08X\n", regs->epc );
		sceIoWrite( fd, buf, strlen( buf ) );
		snprintf( buf, sizeof( buf ), "Cause     - %08X\n", regs->cause );
		sceIoWrite( fd, buf, strlen( buf ) );
		snprintf( buf, sizeof( buf ), "Status    - %08X\n", regs->status );
		sceIoWrite( fd, buf, strlen( buf ) );
		snprintf( buf, sizeof( buf ), "BadVAddr  - %08X\n", regs->badvaddr );
		sceIoWrite( fd, buf, strlen( buf ) );
		snprintf( buf, sizeof( buf ), "BaseAddr  - %08X\n", baseaddr );
		sceIoWrite( fd, buf, strlen( buf ) );
		for(i = 0; i < 32; i+=4)
		{
			snprintf( buf, sizeof( buf ), "%s:%08X %s:%08X %s:%08X %s:%08X\n",
				regName[i],
				regs->r[i],
				regName[i+1],
				regs->r[i+1],
				regName[i+2],
				regs->r[i+2],
				regName[i+3],
				regs->r[i+3]
			);
			sceIoWrite( fd, buf, strlen( buf ) );
		}
		sceIoClose( fd );
	}
}
#endif

int module_start( SceSize arglen, void *argp )
{
	SceUID thid;
	
	dbgprintf( "MacroFire %s - Build %s", MF_VERSION, __DATE__ );
	dbgprint( "pen@classg (http://classg.sytes.net)" );
	dbgprint( "------------------------------------" );
	
#ifdef MF_WITH_EXCEPTION_HANDLER
	pspDebugInstallErrorHandler( mf_exception );
#endif
	
	thid = sceKernelCreateThread( "MacroFire", mf_main, 32, 0xF00, 0, 0 );
	if( thid > 0 ) sceKernelStartThread( thid, arglen, argp );
	
	return 0;
}

int module_stop( SceSize arglen, void *argp )
{
	gRunning = false;
	
	return 0;
}

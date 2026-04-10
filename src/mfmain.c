/*==========================================================

	mfmain.c

	MacroFire メインループ

==========================================================*/
#include "mfmain.h"
#include "ovmsg.h"
#include <pspinit.h>
#include <pspumd.h>
#include <psputils.h>

PSP_MODULE_INFO( "MacroFire", PSP_MODULE_KERNEL, 3, 0 );

/*=========================================================
	定数
=========================================================*/
#define MF_EBOOTPBP_OFFSET_TO_PARAMSFO 8
#define MF_PARAMSFO_OFFSET_TO_DATA     12
#define MF_DATA_OFFSET_TO_GAMEID       8

/*=========================================================
	ローカル関数
=========================================================*/
static void mf_init( void );
static void mf_shutdown( void );
static void mf_get_gameid( SceUID fd, char *gameid, unsigned char *buf );
static void mf_get_gameid_from_sha1hash( SceUID fd, char *gameid );
static void mf_ini( const char *path );
static void mf_ready( void );
static void mf_find_hook_address( void );
static void mf_sending_toggle_message( bool engine );
const char *mf_ini_find_loaded_section( IniUID ini, MfWorldId world, const char *game_id );
static void mf_ini_load( IniUID ini );
static void mf_ini_save( IniUID ini );
static int mf_get_ctrl_buffer( MfHookAction action, unsigned int api, SceCtrlData *pad, MfHprmKey *hk );
static void mf_ctrl_data_copy( SceCtrlData *padarr, int count );

/*=========================================================
	ローカル変数
=========================================================*/
static char st_game_id[10];
static const char *st_ini_loaded_section;

static MfWorldId    st_world        = 0;
static unsigned int st_prev_buttons = 0;
static u32          st_prev_hprmkey = 0;

static bool           st_hooked          = false;
static bool           st_hook_incomplete = false;

static bool           st_is_enabled          = MF_INI_STARTUP;
static PadutilButtons st_menu_buttons        = MF_INI_MENU_BUTTONS;
static PadutilButtons st_toggle_buttons      = MF_INI_TOGGLE_BUTTONS;
static bool           st_status_notification = MF_INI_STATUS_NOTIFICATION;

static PadutilButtonName *st_cvbtn;

static SceCtrlData st_pad;
static MfHprmKey   st_hk;

/*-----------------------------------------------
	MacroFireの初期化と終了
-----------------------------------------------*/
static void mf_init( void )
{
	int i;
	MfFuncInit initfunc;
	unsigned char buf[] ={ 0, 0, 0, 0 };
	const char *executable = sceKernelInitFileName();
	
	/* 最初に行う */
	dbgprint( "Initializing menu" );
	if( ! mfMenuInit() ) gRunning = false;
	
	/* 実行環境取得 */
	switch( sceKernelInitKeyConfig() ){
		case PSP_INIT_KEYCONFIG_VSH:  st_world = MF_WORLD_VSH;  break;
		case PSP_INIT_KEYCONFIG_POPS: st_world = MF_WORLD_POPS; break;
		case PSP_INIT_KEYCONFIG_GAME: st_world = MF_WORLD_GAME; break;
	}
	
	/* ゲームIDを取得 */
	if( st_world == MF_WORLD_VSH ){
		strutilCopy( st_game_id, MF_INI_SECTION_VSH, 10 );
	} else if( st_world == MF_WORLD_GAME && sceKernelBootFrom() == PSP_BOOT_DISC && sceKernelInitApitype() != PSP_INIT_APITYPE_DISC_UPDATER  ){
		SceUID sfo;
		
		/* UMDをdisc0:へマウント */
		sceUmdActivate( 1, "disc0:" );
		
		/* 初期化終了を待つ */
		sceUmdWaitDriveStat( UMD_WAITFORINIT );
		
		sfo = sceIoOpen( "disc0:/PSP_GAME/PARAM.SFO", PSP_O_RDONLY, 0777 ); 
		if( ! ( sfo < 0 ) ){
			mf_get_gameid( sfo, st_game_id, buf );
			sceIoClose( sfo );
		}
		
		/* PSPのオリジナルのローダーがUMDをマウントするはずなので、アンマウント */
		sceUmdDeactivate( 1, "disc0:" );
	} else if( st_world == MF_WORLD_POPS ){
		SceUID pbp = sceIoOpen( executable, PSP_O_RDONLY, 0777 ); 
		if( ! ( pbp < 0 ) ){
			sceIoLseek( pbp, MF_EBOOTPBP_OFFSET_TO_PARAMSFO, PSP_SEEK_SET );
			sceIoRead( pbp, buf, 4 );
			sceIoLseek( pbp, *((unsigned int *)buf), PSP_SEEK_SET );
			mf_get_gameid( pbp, st_game_id, buf );
			sceIoClose( pbp );
		}
	} else if( st_world != MF_WORLD_VSH && executable ){
		SceUID pbp = sceIoOpen( executable, PSP_O_RDONLY, 0777 );
		if( ! ( pbp < 0 ) ){
			mf_get_gameid_from_sha1hash( pbp, st_game_id );
			sceIoClose( pbp );
		}
	}
	
	/* ファンクションの初期化 */
	dbgprint( "Sending message MF_MS_INIT to all functions..." );
	
	initfunc = mfAnalogStickProc( MF_MS_INIT );
	if( initfunc ) initfunc();
	
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
	
	dbgprint( "Sending message MF_MS_TERM to all functions..." );
	for( i = 0; i < gMftabEntryCount; i++ ){
		if( gMftab[i].proc ){
			termfunc = ( gMftab[i].proc )( MF_MS_TERM );
			if( termfunc ) termfunc();
		}
	}
	
	if( mfNotificationThreadId() ) mfNotificationShutdownStart();
	
	mfMenuDestroy();
}

static void mf_get_gameid_from_sha1hash( SceUID fd, char *gameid )
{
	SceKernelUtilsSha1Context ctx;
	int readbytes;
	uint8_t digest[20];
	uint8_t buf[2048]; /* MacroFireのメインスレッドではなくmodule_startから呼び出すことによって、3KBほどのスタックがある */
	
	sceIoLseek( fd, 0, PSP_SEEK_SET );
	
	sceKernelUtilsSha1BlockInit( &ctx );
	
	while( ( readbytes = sceIoRead( fd, buf, sizeof( buf ) ) ) )
		sceKernelUtilsSha1BlockUpdate( &ctx, buf, readbytes );
	
	if( sceKernelUtilsSha1BlockResult( &ctx, digest ) < 0 ) return;
	snprintf( gameid, 10, "%08X", (digest[3]) | (digest[2]) << 8 | (digest[1]) << 16 | (digest[0]) << 24 ); 

}

static void mf_get_gameid( SceUID fd, char *gameid, unsigned char *buf )
{
	sceIoLseek( fd, MF_PARAMSFO_OFFSET_TO_DATA, PSP_SEEK_CUR );
	sceIoRead( fd, buf, 4 );
	sceIoLseek( fd, *((unsigned int *)buf) - ( MF_PARAMSFO_OFFSET_TO_DATA + 4 ) + MF_DATA_OFFSET_TO_GAMEID, PSP_SEEK_CUR );
	sceIoRead( fd, gameid, 10 );
	
	if( gameid[0] == '\0' ) mf_get_gameid_from_sha1hash( fd, gameid );
}

/*-----------------------------------------------
	メインスレッド開始時に最初に呼ばれる。
-----------------------------------------------*/
static void mf_ready( void )
{
	if( st_world == MF_WORLD_VSH ){
		/* VshBridge_Driverを待つ */
		while( sceKernelFindModuleByName( "sceVshBridge_Driver" ) == NULL ) sceKernelDelayThread( 2000000 );
	}
}

/*-----------------------------------------------
	mf_ready()終了後、フックアドレスを取得。
-----------------------------------------------*/
static void mf_find_hook_address( void )
{
	/* フックアドレスを取得 */
	unsigned int i;
	
	dbgprint( "Find hook address..." );
	for( i = 0; i < hooktableEntryCount; i++ ){
		hooktable[i].restore.addr = hookFindSyscallAddr( hookFindExportAddr( hooktable[i].module, hooktable[i].library, hooktable[i].nid ) );
		dbgprintf( "\tNID:0x%08x Addr:%p Orig:%p Hook:%p", hooktable[i].nid, hooktable[i].restore.addr, hooktable[i].restore.addr ? *(hooktable[i].restore.addr): NULL, hooktable[i].hook );
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
}

const char *mf_ini_find_loaded_section( IniUID ini, MfWorldId world, const char *game_id )
{
	const char *section = NULL;
	
	if( inimgrExistSection( ini, game_id ) ) return game_id;
	
	switch( world ){
		case MF_WORLD_VSH:  section = MF_INI_SECTION_VSH;  break;
		case MF_WORLD_GAME: section = MF_INI_SECTION_GAME; break;
		case MF_WORLD_POPS: section = MF_INI_SECTION_POPS; break;
	}
	
	return inimgrExistSection( ini, section ) ? section : MF_INI_SECTION_DEFAULT;
}

/*-----------------------------------------------
	INIの読み込みと保存
-----------------------------------------------*/
static void mf_ini( const char *path )
{
	IniUID ini;
	char inipath[MF_PATH_MAX];
	
	/* 設定ファイルのパスを決定 */
	{
		char *delim;
		
		strutilCopy(
			inipath,
			( ( ! path || path[0] == '\0' || strncasecmp( path, "flash", 5 ) == 0 || ! strrchr( path, '/' ) ) ? MF_INI_PATH_DEFAULT : path ),
			sizeof( inipath ) - strlen( MF_INI_FILENAME )
		);
		delim = strrchr( inipath, '/' );
		strcpy( delim + 1, MF_INI_FILENAME );
	}
	
	st_ini_loaded_section = MF_INI_LOAD_FAILED;
	
	ini = inimgrNew();
	if( ! ini ){
		dbgprintf( "Failed to create IniUID: %x", ini );
		return;
	} else{
		if( mfConvertButtonReady() ){
			if( inimgrLoad( ini, inipath, NULL, 0, 0, 0 ) == 0 ){
				dbgprintf( "%s found", inipath );
				/* ロードに成功したので、ターゲットセクションが存在するかを確認 */
				st_ini_loaded_section = mf_ini_find_loaded_section( ini, st_world, st_game_id );
				mf_ini_load( ini );
			} else{
				dbgprintf( "%s not found", inipath );
				mf_ini_save( ini );
				if( inimgrSave( ini, inipath, NULL, 0, 0, 0 ) == 0 ){
					st_ini_loaded_section = MF_INI_SECTION_DEFAULT;
				}
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
	const char *section = st_ini_loaded_section;
	MfFuncIni inifunc;
	
	/* 設定値をロード */
	inimgrGetBool( ini, section, "Startup", &st_is_enabled );
	if( inimgrGetString( ini, section, "MenuButtons", buf, sizeof( buf ) ) > 0 )   st_menu_buttons = mfConvertButtonN2C( buf );
	if( inimgrGetString( ini, section, "ToggleButtons", buf, sizeof( buf ) ) > 0 ) st_toggle_buttons = mfConvertButtonN2C( buf );
	inimgrGetBool( ini, section, "StatusNotification", &st_status_notification );
	
	mfAnalogStickIniLoad( ini, buf, sizeof( buf ) );
	mfMenuIniLoad( ini, buf, sizeof( buf ) );

	dbgprint( "Sending message MF_MS_INI_LOAD to all functions..." );
	for( i = 0; i < gMftabEntryCount; i++ ){
		if( gMftab[i].proc ){
			inifunc = ( gMftab[i].proc )( MF_MS_INI_LOAD );
			if( inifunc ) inifunc( ini, buf, sizeof( buf ) );
		}
	}
}

static void mf_ini_save( IniUID ini )
{
	int i;
	char buf[255];
	MfFuncIni inifunc;
	
	inimgrSetBool( ini, MF_INI_SECTION_DEFAULT, "Startup", st_is_enabled );
	
	mfConvertButtonC2N( st_menu_buttons, buf, sizeof( buf ) );
	inimgrSetString( ini, MF_INI_SECTION_DEFAULT, "MenuButtons", buf );
	
	mfConvertButtonC2N( st_toggle_buttons, buf, sizeof( buf ) );
	inimgrSetString( ini, MF_INI_SECTION_DEFAULT, "ToggleButtons", buf );
	
	inimgrSetBool( ini, MF_INI_SECTION_DEFAULT, "StatusNotification", MF_INI_STATUS_NOTIFICATION );
	
	mfAnalogStickIniSave( ini, buf, sizeof( buf ) );
	mfMenuIniSave( ini, buf, sizeof( buf ) );
	
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
	int ret;
	MfFuncHook hookfunc;
	
	if( hooktable[api].restore.api ){
		unsigned int k1_register = pspSdkSetK1( 0 );
		ret = ( ( int (*)( SceCtrlData*, int ) )hooktable[api].restore.api )( pad, 1 );
		if( hooktable[MF_HPRM_PEEK_CURRENT_KEY].restore.api ) ( ( int (*)( u32* ) )hooktable[MF_HPRM_PEEK_CURRENT_KEY].restore.api )( hk );
		pspSdkSetK1( k1_register );
	} else{
		switch( api ){
			case MF_CTRL_PEEK_BUFFER_POSITIVE:    ret = sceCtrlPeekBufferPositive( pad, 1 ); break;
			case MF_CTRL_PEEK_BUFFER_NEGATIVE:    ret = sceCtrlPeekBufferNegative( pad, 1 ); break;
			case MF_CTRL_READ_BUFFER_POSITIVE:    ret = sceCtrlReadBufferPositive( pad, 1 ); break;
			case MF_CTRL_READ_BUFFER_NEGATIVE:    ret = sceCtrlReadBufferNegative( pad, 1 ); break;
			case MF_VSHCTRL_READ_BUFFER_POSITIVE: ret = sceCtrlReadBufferPositive( pad, 1 ); break;
			default: return CG_ERROR_INVALID_ARGUMENT;
		}
		*hk = 0;
	}
	
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
		pad->Buttons &= MF_TARGET_BUTTONS;
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
	
	if( mfNotificationThreadId() ){
		mfNotificationPrintf( "%s: %s", MF_STR_HOME_MACROFIRE_ENGINE, engine ? MF_STR_CTRL_ON : MF_STR_CTRL_OFF );
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
	
	mf_ready();
	
	mf_find_hook_address();
	
	if( st_status_notification ) mfNotificationStart();
	
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
	
	latch->uiMake    |= ( st_prev_buttons ^ pad.Buttons ) & pad.Buttons;
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
		padutilDestroyButtonNames();
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

const char *mfGetIniSection( void )
{
	return st_ini_loaded_section;
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

int mfNotificationStart( void )
{
	SceUID msg_thid;
	
	ovmsgInit();
	
	msg_thid = sceKernelCreateThread( "MacroFireNotification", ovmsgThreadMain, 16, 0x300, 0, 0 );
	if( msg_thid > 0 ){
		sceKernelStartThread( msg_thid, 0, NULL );
		ovmsgWaitForReady();
		return 0;
	} else{
		return msg_thid;
	}
}

void mfNotificationShutdownStart( void )
{
	ovmsgShutdownStart();
}

SceUID mfNotificationThreadId( void )
{
	return ovmsgGetThreadId();
}

void mfNotificationPrintTerm( void )
{
	ovmsgPrintIntrStart();
	ovmsgWaitForReady();
}

bool mfNotificationPrintf( const char *format, ... )
{
	va_list ap;
	bool ret;
	unsigned int k1;
	
	va_start( ap, format );
	
	k1 = pspSdkSetK1( 0 );
	mfNotificationPrintTerm();
	ret = ovmsgVprintf( format, ap );
	pspSdkSetK1( k1 );
	
	va_end( ap );
	
	return ret;
}

bool mfHookIncomplete( void )
{
	return st_hook_incomplete;
}

const PadutilAnalogStick *mfGetAnalogStickContext( void )
{
	return mfAnalogStickGetContext();
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
	
	dbgprint( "Initializing" );
	mf_init();
	
	dbgprint( "Checking ini" );
	mf_ini( (const char *)argp );
	
	thid = sceKernelCreateThread( "MacroFire", mf_main, 32, 0x900, 0, 0 );
	if( thid > 0 ) sceKernelStartThread( thid, arglen, argp );
	
	return 0;
}

int module_stop( SceSize arglen, void *argp )
{
	gRunning = false;
	
	return 0;
}

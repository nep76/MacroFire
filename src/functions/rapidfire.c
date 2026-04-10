/*=========================================================

	function/rapidfire.c

	連射設定。

=========================================================*/
#include "rapidfire.h"

/*=========================================================
	ローカル型宣言
=========================================================*/
struct rapidfire_delay {
	unsigned short pressDelay;
	unsigned short releaseDelay;
};

struct rapidfire_mode {
	unsigned int  button;
	rapidfireMode mode;
};

/*=========================================================
	ローカル関数
=========================================================*/
static void rapidfire_apply( struct rapidfire_mode *mode, struct rapidfire_delay *delay );
static void rapidfire_menu_save( MfMenuMessage message );
static void rapidfire_menu_load( MfMenuMessage message );
static bool rapidfire_save( const char *path, struct rapidfire_mode *mode, struct rapidfire_delay *delay );
static bool rapidfire_load( const char *path, struct rapidfire_mode *mode, struct rapidfire_delay *delay );

static const char *rapidfire_get_mode_name_by_idn( unsigned short idn );
static unsigned short rapidfire_get_mode_idn_by_name( const char *name );

/*=========================================================
	ローカル変数
=========================================================*/
static MfRapidfireUID st_rfuid;
static struct rapidfire_mode  *st_mode;
static struct rapidfire_delay st_delay;

static HeapUID     st_heap;

/*=========================================================
	関数
=========================================================*/
void *rapidfireProc( MfMessage message )
{
	if( message != MF_MS_INIT && ! st_rfuid ) return NULL;
	
	switch( message ){
		case MF_MS_INIT:
			st_rfuid = mfRapidfireNew();
			break;
		case MF_MS_INI_LOAD:
			return rapidfireIniLoad;
		case MF_MS_INI_CREATE:
			return rapidfireIniCreate;
		case MF_MS_TERM:
			mfRapidfireDestroy( st_rfuid );
			break;
		case MF_MS_HOOK:
			return rapidfireMain;
		case MF_MS_MENU:
			return rapidfireMenu;
		default:
			break;
	}
	return NULL;
}

void rapidfireIniLoad( IniUID ini, char *buf, size_t len )
{
	const char *section = mfGetIniSection();
	
	/* 設定用メモリ確保 */
	st_mode  = memoryAlloc( sizeof( struct rapidfire_mode ) * MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS );
	if( st_mode ){
		unsigned short i, save;
		
		/* MacroFireコアから設定値を読み出し、ボタンコードを取得 */
		for( i = 0, save = 0; mfRapidfireReadEntry( st_rfuid, &(st_mode[i].button), NULL, NULL, NULL, NULL, &save ); i++ ){
			st_mode[i].mode = RAPIDFIRE_NORMAL;
		}
		
		if( inimgrGetString( ini, section, "Rapidfire", buf, len ) > 0 ){
			rapidfire_load( buf, st_mode, &st_delay );
			rapidfire_apply( st_mode, &st_delay );
		}
		memoryFree( st_mode );
	}
}

void rapidfireIniCreate( IniUID ini, char *buf, size_t len )
{
	inimgrSetString( ini, MF_INI_SECTION_DEFAULT, "Rapidfire", "" );
}

void rapidfireMain( MfHookAction action, SceCtrlData *pad, MfHprmKey *hk )
{
	if( mfIsRunningApp( MF_APP_WEBBROWSER ) ) return;
	mfRapidfireExec( st_rfuid, action, pad );
}

void rapidfireMenu( MfMessage message )
{
	static MfMenuTable *menu;
	
	if( mfIsRunningApp( MF_APP_WEBBROWSER ) ){
		if( message == MF_MM_PROC ){
			pbPrint( pbOffsetChar( 3 ), pbOffsetLine(  4 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_STR_LIMIT_WEBBROWSER );
			mfMenuWait( MF_ERROR_DELAY );
			mfMenuProc( NULL );
		}
		return;
	}
	
	switch( message ){
		case MF_MM_INIT:
			dbgprint( "Creating rapidfire menu" );
			menu = mfMenuCreateTables(
				3,
				7, 2,
				2, 1,
				2, 1
			);
			st_heap = mfHeapCreate( 4,
				sizeof( struct rapidfire_mode ) * 12 +
				sizeof( char ** ) * 6                +
				sizeof( MfCtrlDefGetNumberPref )     +
				MF_PATH_MAX
			);
			if( ! menu || ! st_heap ){
				if( menu ) mfMenuDestroyTables( menu );
				if( st_heap ) mfHeapDestroy( st_heap );
				return;
			}
			
			st_mode  = mfHeapAlloc( st_heap, sizeof( struct rapidfire_mode ) * MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS );
			
			{
				unsigned short i, save;
				MfRapidfireMode mode;
				unsigned int pdelay, rdelay;
				bool autorun;
				
				MfCtrlDefGetNumberPref *delaypref = mfHeapCalloc( st_heap, sizeof( MfCtrlDefGetNumberPref ) );
				const char **options = mfHeapAlloc( st_heap, sizeof( char ** ) * 6 );
				
				delaypref->unit = "ms";
				delaypref->max   = 999;
				
				options[0] = rapidfire_get_mode_name_by_idn( RAPIDFIRE_NORMAL );
				options[1] = rapidfire_get_mode_name_by_idn( RAPIDFIRE_RAPID );
				options[2] = rapidfire_get_mode_name_by_idn( RAPIDFIRE_AUTORAPID );
				options[3] = rapidfire_get_mode_name_by_idn( RAPIDFIRE_HOLD );
				options[4] = rapidfire_get_mode_name_by_idn( RAPIDFIRE_AUTOHOLD );
				options[5] = NULL;
				
				mfMenuSetTablePosition( menu, 1, pbOffsetChar( 5 ), pbOffsetLine( 6 ) );
				mfMenuSetTableLabel( menu, 1, MF_STR_RAPIDFIRE_MODE_LABEL );
				mfMenuSetColumnWidth( menu, 1, 1, 20 );
				
				/* MacroFireコアから設定値を読み出す */
				for( i = 0, save = 0; mfRapidfireReadEntry( st_rfuid, &(st_mode[i].button), &mode, &pdelay, &rdelay, &autorun, &save ); i++ ){
					if( mode == MF_RAPIDFIRE_MODE_RAPID ){
						st_mode[i].mode = autorun ? RAPIDFIRE_AUTORAPID : RAPIDFIRE_RAPID;
					} else if( mode == MF_RAPIDFIRE_MODE_HOLD ){
						st_mode[i].mode = autorun ? RAPIDFIRE_AUTOHOLD : RAPIDFIRE_HOLD;
					} else{
						st_mode[i].mode = RAPIDFIRE_NORMAL;
					}
					
					switch( st_mode[i].button ){
						case PSP_CTRL_TRIANGLE: mfMenuSetTableEntry( menu, 1, 1, 1, MF_STR_RAPIDFIRE_BUTTON_TRIANGLE, mfCtrlDefOptions, &(st_mode[i].mode), options ); break;
						case PSP_CTRL_CIRCLE:   mfMenuSetTableEntry( menu, 1, 2, 1, MF_STR_RAPIDFIRE_BUTTON_CIRCLE,   mfCtrlDefOptions, &(st_mode[i].mode), options ); break;
						case PSP_CTRL_CROSS:    mfMenuSetTableEntry( menu, 1, 3, 1, MF_STR_RAPIDFIRE_BUTTON_CROSS,    mfCtrlDefOptions, &(st_mode[i].mode), options ); break;
						case PSP_CTRL_SQUARE:   mfMenuSetTableEntry( menu, 1, 4, 1, MF_STR_RAPIDFIRE_BUTTON_SQUARE,   mfCtrlDefOptions, &(st_mode[i].mode), options ); break;
						case PSP_CTRL_LTRIGGER: mfMenuSetTableEntry( menu, 1, 6, 1, MF_STR_RAPIDFIRE_BUTTON_LTRIGGER, mfCtrlDefOptions, &(st_mode[i].mode), options ); break;
						case PSP_CTRL_RTRIGGER: mfMenuSetTableEntry( menu, 1, 7, 1, MF_STR_RAPIDFIRE_BUTTON_RTRIGGER, mfCtrlDefOptions, &(st_mode[i].mode), options ); break;
						case PSP_CTRL_UP:       mfMenuSetTableEntry( menu, 1, 1, 2, MF_STR_RAPIDFIRE_BUTTON_UP,       mfCtrlDefOptions, &(st_mode[i].mode), options ); break;
						case PSP_CTRL_RIGHT:    mfMenuSetTableEntry( menu, 1, 2, 2, MF_STR_RAPIDFIRE_BUTTON_RIGHT,    mfCtrlDefOptions, &(st_mode[i].mode), options ); break;
						case PSP_CTRL_DOWN:     mfMenuSetTableEntry( menu, 1, 3, 2, MF_STR_RAPIDFIRE_BUTTON_DOWN,     mfCtrlDefOptions, &(st_mode[i].mode), options ); break;
						case PSP_CTRL_LEFT:     mfMenuSetTableEntry( menu, 1, 4, 2, MF_STR_RAPIDFIRE_BUTTON_LEFT,     mfCtrlDefOptions, &(st_mode[i].mode), options ); break;
						case PSP_CTRL_START:    mfMenuSetTableEntry( menu, 1, 6, 2, MF_STR_RAPIDFIRE_BUTTON_START,    mfCtrlDefOptions, &(st_mode[i].mode), options ); break;
						case PSP_CTRL_SELECT:   mfMenuSetTableEntry( menu, 1, 7, 2, MF_STR_RAPIDFIRE_BUTTON_SELECT,   mfCtrlDefOptions, &(st_mode[i].mode), options ); break;
						default: continue;
					}
				}
				
				mfMenuSetTablePosition( menu, 2, pbOffsetChar( 5 ), pbOffsetLine( 15 ) );
				mfMenuSetTableLabel( menu, 2, MF_STR_RAPIDFIRE_WAITCONTROL_LABEL );
				mfMenuSetTableEntry( menu, 2, 1, 1, MF_STR_RAPIDFIRE_WAITCONTROL_PRESS_DELAY,   mfCtrlDefGetNumber, &(st_delay.pressDelay), delaypref );
				mfMenuSetTableEntry( menu, 2, 2, 1, MF_STR_RAPIDFIRE_WAITCONTROL_RELEASE_DELAY, mfCtrlDefGetNumber, &(st_delay.releaseDelay), delaypref );
				
				mfMenuSetTablePosition( menu, 3, pbOffsetChar( 5 ), pbOffsetLine( 19 ) );
				mfMenuSetTableLabel( menu, 3, MF_STR_RAPIDFIRE_FILE_LABEL );
				mfMenuSetTableEntry( menu, 3, 1, 1, MF_STR_LOAD, mfCtrlDefExtra, rapidfire_menu_load, NULL );
				mfMenuSetTableEntry( menu, 3, 2, 1, MF_STR_SAVE, mfCtrlDefExtra, rapidfire_menu_save, NULL );
			}
			mfMenuInitTables( menu, 3 );
			break;
		case MF_MM_TERM:
			rapidfire_apply( st_mode, &st_delay );
			mfMenuDestroyTables( menu );
			mfHeapDestroy( st_heap );
			return;
		default:
			pbPrint( pbOffsetChar( 3 ), pbOffsetLine(  4 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_STR_RAPIDFIRE_DESC );
			pbPrint( pbOffsetChar( 3 ), pbOffsetLine( 25 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_STR_RAPIDFIRE_MANUAL_NORMAL );
			pbPrint( pbOffsetChar( 3 ), pbOffsetLine( 26 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_STR_RAPIDFIRE_MANUAL_RAPID );
			pbPrint( pbOffsetChar( 3 ), pbOffsetLine( 27 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_STR_RAPIDFIRE_MANUAL_AUTORAPID );
			pbPrint( pbOffsetChar( 3 ), pbOffsetLine( 28 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_STR_RAPIDFIRE_MANUAL_HOLD );
			pbPrint( pbOffsetChar( 3 ), pbOffsetLine( 29 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_STR_RAPIDFIRE_MANUAL_AUTOHOLD );
			if( ! mfMenuDrawTables( menu, 3, MF_MENU_NO_OPTIONS ) ) mfMenuProc( NULL );
	}
}

static const char *rapidfire_get_mode_name_by_idn( unsigned short idn )
{
	switch( idn ){
		case RAPIDFIRE_RAPID:     return mfRapidfireGetNameByMode( MF_RAPIDFIRE_MODE_RAPID, false );
		case RAPIDFIRE_AUTORAPID: return mfRapidfireGetNameByMode( MF_RAPIDFIRE_MODE_RAPID, true );
		case RAPIDFIRE_HOLD:      return mfRapidfireGetNameByMode( MF_RAPIDFIRE_MODE_HOLD, false );
		case RAPIDFIRE_AUTOHOLD:  return mfRapidfireGetNameByMode( MF_RAPIDFIRE_MODE_HOLD, true );
		default:                  return mfRapidfireGetNameByMode( MF_RAPIDFIRE_MODE_NORMAL, false );
	}
}

static unsigned short rapidfire_get_mode_idn_by_name( const char *name )
{
	MfRapidfireMode mode;
	bool autorun;
	
	mfRapidfireGetModeByName( name, &mode, &autorun );
	
	switch( mode ){
		case MF_RAPIDFIRE_MODE_RAPID:
			return autorun ? RAPIDFIRE_AUTORAPID : RAPIDFIRE_RAPID;
		case MF_RAPIDFIRE_MODE_HOLD:
			return autorun ? RAPIDFIRE_AUTOHOLD : RAPIDFIRE_HOLD;
		default:
			return RAPIDFIRE_NORMAL;
	}
}

static void rapidfire_apply( struct rapidfire_mode *mode, struct rapidfire_delay *delay )
{
	unsigned short i;
	unsigned int buttons_normal    = 0;
	unsigned int buttons_rapid     = 0;
	unsigned int buttons_autorapid = 0;
	unsigned int buttons_hold      = 0;
	unsigned int buttons_autohold  = 0;
	
	for( i = 0; i < 12; i++ ){
		switch( mode[i].mode ){
			case RAPIDFIRE_RAPID:
				buttons_rapid |= mode[i].button;
				break;
			case RAPIDFIRE_AUTORAPID:
				buttons_autorapid |= mode[i].button;
				break;
			case RAPIDFIRE_HOLD:
				buttons_hold |= mode[i].button;
				break;
			case RAPIDFIRE_AUTOHOLD:
				buttons_autohold |= mode[i].button;
				break;
			default:
				buttons_normal |= mode[i].button;
				break;
		}
	}
	
	if( buttons_normal    ) mfRapidfireClear( st_rfuid, buttons_normal );
	if( buttons_rapid     ) mfRapidfireSetRapid( st_rfuid, buttons_rapid,     delay->pressDelay, delay->releaseDelay, false );
	if( buttons_autorapid ) mfRapidfireSetRapid( st_rfuid, buttons_autorapid, delay->pressDelay, delay->releaseDelay, true );
	if( buttons_hold      ) mfRapidfireSetHold( st_rfuid, buttons_hold,     false );
	if( buttons_autohold  ) mfRapidfireSetHold( st_rfuid, buttons_autohold, true );
}

static void rapidfire_menu_save( MfMenuMessage message )
{
	static char *path;
	
	if( message == MF_MM_INIT ){
		path = mfHeapAlloc( st_heap, MF_PATH_MAX );
		
		if( ! mfDialogGetfilenameInit( MF_STR_RAPIDFIRE_DIAGGETFN_SAVE, "ms0:", "rapidfire.ini", path, 255, CDIALOG_GETFILENAME_SAVE | CDIALOG_GETFILENAME_OVERWRITEPROMPT  ) ){
			mfHeapFree( st_heap, path );
			path = NULL;
			mfMenuExitExtra();
		}
	} else if( message == MF_MM_PROC ){
		if( mfDialogCurrentType() ){
			if( ! mfDialogGetfilenameDraw() ){
				if( ! mfDialogGetfilenameResult() ){
					mfHeapFree( st_heap, path );
					path = NULL;
				}
			}
		} else if( path ){
			mfMenuSetInfoText( MF_MENU_INFOTEXT_SET_MIDDLE_LINE, MF_STR_SAVING, path );
			rapidfire_save( path, st_mode, &st_delay );
			mfHeapFree( st_heap, path );
			path = NULL;
		} else if( ! path ){
			mfMenuExitExtra();
		}
	}
}

static void rapidfire_menu_load( MfMenuMessage message )
{
	static char *path;
	
	if( message == MF_MM_INIT ){
		path = mfHeapAlloc( st_heap, MF_PATH_MAX );
		
		if( ! mfDialogGetfilenameInit( MF_STR_RAPIDFIRE_DIAGGETFN_LOAD, "ms0:", "", path, 255, CDIALOG_GETFILENAME_OPEN | CDIALOG_GETFILENAME_FILEMUSTEXIST ) ){
			mfHeapFree( st_heap, path );
			path = NULL;
			mfMenuExitExtra();
		}
	} else if( message == MF_MM_PROC ){
		if( mfDialogCurrentType() ){
			if( ! mfDialogGetfilenameDraw() ){
				if( ! mfDialogGetfilenameResult() ){
					mfHeapFree( st_heap, path );
					path = NULL;
				}
			}
		} else if( path ){
			mfMenuSetInfoText( MF_MENU_INFOTEXT_SET_MIDDLE_LINE, MF_STR_LOADING, path );
			rapidfire_load( path, st_mode, &st_delay );
			mfHeapFree( st_heap, path );
			path = NULL;
		} else if( ! path ){
			mfMenuExitExtra();
		}
	}
}

static bool rapidfire_save( const char *path, struct rapidfire_mode *mode, struct rapidfire_delay *delay )
{
	IniUID ini = inimgrNew();
	if( ini ){
		int  i, ret;
		char name[16];
		
		if( mfConvertButtonReady() ){
			for( i = 0; i < MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS; i++ ){
				inimgrSetString( ini, RAPIDFIRE_INI_SECTION_MODE, mfConvertButtonC2N( mode[i].button, name, sizeof( name ) ), rapidfire_get_mode_name_by_idn( mode[i].mode ) );
			}
			mfConvertButtonFinish();
			
			inimgrSetInt( ini, RAPIDFIRE_INI_SECTION_DELAY, RAPIDFIRE_INI_ENTRY_PD, delay->pressDelay );
			inimgrSetInt( ini, RAPIDFIRE_INI_SECTION_DELAY, RAPIDFIRE_INI_ENTRY_RD, delay->releaseDelay );
			
			ret = inimgrSave( ini, path, RAPIDFIRE_INI_SIGNATURE, RAPIDFIRE_INI_VERSION, 0, 0 );
			inimgrDestroy( ini );
			
			if( ret != 0 ){
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "inimgr %s: %X", MF_STR_ERROR, ret );
				mfMenuWait( MF_ERROR_DELAY );
			} else{
				return true;
			}
		} else{
			mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "%s: %s.", MF_STR_ERROR, MF_STR_ERROR_NOT_ENOUGH_MEMORY );
			mfMenuWait( MF_ERROR_DELAY );
		}
		inimgrDestroy( ini );
	} else{
		mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "inimgr %s: %s.", MF_STR_ERROR, MF_STR_ERROR_NOT_ENOUGH_MEMORY );
		mfMenuWait( MF_ERROR_DELAY );
	}
	
	return false;
}

static bool rapidfire_load( const char *path, struct rapidfire_mode *mode, struct rapidfire_delay *delay )
{
	IniUID ini = inimgrNew();
	if( ini ){
		int  i, ret;
		
		ret = inimgrLoad( ini, path, RAPIDFIRE_INI_SIGNATURE, RAPIDFIRE_INI_VERSION, 0, 0 );
		if( ret != 0 ){
			if( ret == INIMGR_ERROR_INVALID_SIGNATURE ){
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "%s: %s.", MF_STR_ERROR, MF_STR_ERROR_INVALID_CONF );
			} else if( ret == INIMGR_ERROR_INVALID_VERSION ){
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "%s: %s.", MF_STR_ERROR, MF_STR_ERROR_UNSUPPORTED_VERSION );
			} else {
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "inimgr %s: %X", MF_STR_ERROR, ret );
			}
			mfMenuWait( MF_ERROR_DELAY );
		} else if( mfConvertButtonReady() ){
			char button_name[16];
			char mode_name[16];
			int delayms;
			
			for( i = 0; i < MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS; i++ ){
				if( inimgrGetString( ini, RAPIDFIRE_INI_SECTION_MODE, mfConvertButtonC2N( mode[i].button, button_name, sizeof( button_name ) ), mode_name, sizeof( mode_name ) ) > 0 ){
					mode[i].mode = rapidfire_get_mode_idn_by_name( mode_name );
				} else{
					mode[i].mode = RAPIDFIRE_NORMAL;
				}
			}
			mfConvertButtonFinish();
			
			if( inimgrGetInt( ini, RAPIDFIRE_INI_SECTION_DELAY, RAPIDFIRE_INI_ENTRY_PD, &delayms ) ) delay->pressDelay = delayms;
			if( inimgrGetInt( ini, RAPIDFIRE_INI_SECTION_DELAY, RAPIDFIRE_INI_ENTRY_RD, &delayms ) ) delay->releaseDelay = delayms;
			
			inimgrDestroy( ini );
			return true;
		} else{
			mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "%s: %s.", MF_STR_ERROR, MF_STR_ERROR_NOT_ENOUGH_MEMORY );
			mfMenuWait( MF_ERROR_DELAY );
		}
		inimgrDestroy( ini );
	} else{
		mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "inimgr %s: %s.", MF_STR_ERROR, MF_STR_ERROR_NOT_ENOUGH_MEMORY );
		mfMenuWait( MF_ERROR_DELAY );
	}
	
	return false;
}

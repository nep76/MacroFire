/*=========================================================

	macro.c

	マクロ。

=========================================================*/
#include "macro.h"

/*=========================================================
	ローカル型宣言
=========================================================*/
enum macro_error {
	MACRO_ERROR_NONE           = 0,
	MACRO_ERROR_BUSY           = 0x00000001,
	MACRO_ERROR_UNAVAIL        = 0x00000002,
	MACRO_ERROR_DISABLE_ENGINE = 0x00000004
};

enum macro_mode {
	MACRO_NTD = 0,
	MACRO_TRACE,
	MACRO_RECORD,
};

struct macro_common_tempdata {
	unsigned int  lastButtons;
	unsigned char analogX;
	unsigned char analogY;
	uint64_t      rtc;
};

struct macro_trace_tempdata {
	MfRapidfireUID rfUid;
	int            loopRest;
};

struct macro_params {
	enum macro_mode mode;
	MacromgrUID  macro;
	char name[MACRO_NAME_LENGTH];
	PadutilButtons hotkey;
	unsigned int loopNum;
	bool analogStickEnable;
	struct {
		struct macro_common_tempdata common;
		struct macro_trace_tempdata trace;
		bool hotkeyAccepted;
		MacromgrCommand *cmd;
	} temp;
};

/*=========================================================
	ローカル関数
=========================================================*/
static void macro_select( unsigned short slot );

static bool macro_slot_control( MfMessage message, const char *label, void *var, void *pref, void *ex );
static bool macro_hotkey_control( MfMessage message, const char *label, void *var, void *pref, void *ex );
static bool macro_loop_control( MfMessage message, const char *label, void *var, void *pref, void *ex );
static bool macro_analogstick_control( MfMessage message, const char *label, void *var, void *pref, void *ex );

static void macro_menu_record_start( MfMenuMessage message );
static void macro_menu_append_start( MfMenuMessage message );
static void macro_menu_record_stop( MfMenuMessage message );
static void macro_menu_run( MfMenuMessage message );
static void macro_menu_stop( MfMenuMessage message );
static void macro_menu_edit( MfMenuMessage message );
static void macro_menu_clear( MfMenuMessage message );
static void macro_menu_save( MfMenuMessage message );
static void macro_menu_load( MfMenuMessage message );

static void macro_clear( struct macro_params *slot );
static bool macro_save( struct macro_params *slot, const char *path );
static bool macro_load( struct macro_params *slot, const char *path );

static void macro_run( struct macro_params *params );
static bool macro_loop( struct macro_params *params );
static unsigned int macro_calc_delayms( MacromgrData base );
static void macro_record_prepare( struct macro_params *params, MacromgrCommand *start );
static void macro_record_stop( struct macro_params *params );
static void macro_record( struct macro_params *params, SceCtrlData *pad );
static void macro_trace_prepare( struct macro_params *params, MacromgrCommand *start );
static void macro_trace_loop( struct macro_params *params, MacromgrCommand *start );
static void macro_trace_stop( struct macro_params *params );
static bool macro_trace( struct macro_params *params, SceCtrlData *pad );

static void macro_mfengine_warning( void );

/*=========================================================
	ローカル変数
=========================================================*/
static unsigned short st_current_slot = 0;
static struct macro_params st_slot[MACRO_MAX_SLOT];

static MfMenuTable *st_menu;
static HeapUID     st_heap;

/*=========================================================
	関数
=========================================================*/
void *macroProc( MfMessage message )
{
	switch( message ){
		case MF_MS_INIT:
			break;
		case MF_MS_INI_LOAD:
			return macroIniLoad;
		case MF_MS_INI_CREATE:
			return macroIniCreate;
		case MF_MS_TERM:
			break;
		case MF_MS_TOGGLE:
			{
				int i;
				for( i = 0; i < MACRO_MAX_SLOT; i++ ){
					if( st_slot[i].mode == MACRO_TRACE ) macro_trace_stop( &(st_slot[i]) );
				}
			}
			break;
		case MF_MS_HOOK:
			return macroMain;
		case MF_MS_MENU:
			return macroMenu;
		default:
			break;
	}
	
	return NULL;
}

void macroIniLoad( IniUID ini, char *buf, size_t len )
{
	unsigned short entry_i;
	char entryname[32];
	const char *section = mfGetIniTargetSection();
	
	for( entry_i = 0; entry_i < MACRO_MAX_SLOT; entry_i++ ){
		/* エントリ名を作成 */
		snprintf( entryname, sizeof( entryname ), "Macro%d", entry_i + 1 );
		
		if( inimgrGetString( ini, section, entryname, buf, len ) > 0 ){
			macro_load( &(st_slot[entry_i]), buf );
		}
	}
}

void macroIniCreate( IniUID ini, char *buf, size_t len )
{
	unsigned int i;
	
	for( i = 1; i <= MACRO_MAX_SLOT; i++ ){
		snprintf( buf, len, "Macro%d", i );
		inimgrSetString( ini, MF_INI_SECTION_DEFAULT, buf, "" );
	}
}

void macroMain( MfHookAction action, SceCtrlData *pad, MfHprmKey *hk )
{
	unsigned short i;
	
	enum {
		MACRO_READY = 0,
		MACRO_RECORD_WORK,
		MACRO_TRACE_START,
		MACRO_TRACE_WORK,
		MACRO_TRACE_STOP,
	} mode = MACRO_READY;
	unsigned short  target_slot = 0;
	
	if( mfIsRunningApp( MF_APP_WEBBROWSER ) ) return;
	
	if( action == MF_KEEP ){
		for( i = 0; i < MACRO_MAX_SLOT; i++ ){
			if( st_slot[i].mode == MACRO_TRACE ){
				pad->Buttons |= st_slot[i].temp.common.lastButtons;
				break;
			}
		}
		return;
	}
	
	for( i = 0; i < MACRO_MAX_SLOT; i++ ){
		bool hotkey_detected = false;
		
		if( ! st_slot[i].macro ) continue;
		
		if( st_slot[i].hotkey && mfIsEnabled() ){
			bool trigger = ( ( padutilSetPad( pad->Buttons | padutilGetAnalogStickDirection( pad->Lx, pad->Ly, 0 ) ) | padutilSetHprm( *hk ) ) & st_slot[i].hotkey ) == st_slot[i].hotkey ? true : false;
			
			if( ! trigger && ! st_slot[i].temp.hotkeyAccepted ){
				st_slot[i].temp.hotkeyAccepted = true;
			} else if( trigger && st_slot[i].temp.hotkeyAccepted ){
				hotkey_detected = true;
				st_slot[i].temp.hotkeyAccepted = false;
			}
		}
		
		if( mode != MACRO_RECORD_WORK && st_slot[i].mode == MACRO_RECORD ){
			mode = MACRO_RECORD_WORK;
			target_slot = i;
			break;
		} else if( mode == MACRO_READY ){
			if( hotkey_detected ){
				if( st_slot[i].mode == MACRO_NTD ){
					mode = MACRO_TRACE_START;
					target_slot = i;
				} else{
					mode = MACRO_TRACE_STOP;
					target_slot = i;
				}
			} else if( st_slot[i].mode == MACRO_TRACE ){
				mode = MACRO_TRACE_WORK;
				target_slot = i;
			}
		} else if( mode == MACRO_TRACE_START && st_slot[i].mode == MACRO_TRACE && hotkey_detected ){
			mode = MACRO_TRACE_STOP;
			target_slot = i;
		}
	}
	
	switch( mode ){
		case MACRO_RECORD_WORK:
			macro_record( &(st_slot[target_slot]), pad );
			break;
		case MACRO_TRACE_START:
			macro_run( &(st_slot[target_slot]) );
			macro_trace( &(st_slot[target_slot]), pad );
			break;
		case MACRO_TRACE_WORK:
			if( ! macro_trace( &(st_slot[target_slot]), pad ) ){
				if( macro_loop( &(st_slot[target_slot]) ) ){
					macro_trace( &(st_slot[target_slot]), pad );
				} else{
					macro_trace_stop(  &(st_slot[target_slot]) );
				}
			}
			break;
		case MACRO_TRACE_STOP:
			macro_trace_stop(  &(st_slot[target_slot]) );
			break;
		default:
			break;
	}
}

void macroMenu( MfMenuMessage message )
{
	if( mfIsRunningApp( MF_APP_WEBBROWSER ) ){
		if( message == MF_MM_PROC ){
			gbPrint( gbOffsetChar( 3 ), gbOffsetLine(  4 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "Cannot use this function on the web-browser." );
			mfMenuWait( MF_ERROR_DELAY );
			mfMenuProc( NULL );
		}
		return;
	}
	
	switch( message ){
		case MF_MM_INIT:
			dbgprint( "Creating macro menu" );
			st_menu = mfMenuCreateTables(
				5,
				1, 1, /* slot */
				2, 1, /* preference */
				4, 1, /* control */
				4, 1, /* recording */
				2, 1  /* save/load */
			);
			st_heap = mfHeapCreate( 5,
				sizeof( MfCtrlDefGetButtonsPref )     +
				sizeof( MfCtrlDefGetNumberPref  ) * 2 +
				MF_PATH_MAX                           +
				MACRO_NAME_LENGTH + 4
			);
			if( ! st_menu || ! st_heap ){
				if( st_menu ) mfMenuDestroyTables( st_menu );
				if( st_heap ) mfHeapDestroy( st_heap );
				return;
			}
			
			{
				MfCtrlDefGetNumberPref  *pref_slot   = mfHeapCalloc( st_heap, sizeof( MfCtrlDefGetNumberPref ) );
				MfCtrlDefGetButtonsPref *pref_hotkey = mfHeapCalloc( st_heap, sizeof( MfCtrlDefGetButtonsPref ) );
				MfCtrlDefGetNumberPref  *pref_loop   = mfHeapCalloc( st_heap, sizeof( MfCtrlDefGetNumberPref ) );
				
				pref_slot->max            = MACRO_MAX_SLOT -1;
				pref_hotkey->availButtons = MF_HOTKEY_BUTTONS;
				pref_loop->max            = 99999;
				
				mfMenuSetTablePosition( st_menu, 1, gbOffsetChar( 5 ), gbOffsetLine( 6 ) );
				mfMenuSetTableEntry( st_menu, 1, 1, 1, "Macro slot", macro_slot_control, &st_current_slot, pref_slot );
				
				mfMenuSetTablePosition( st_menu, 2, gbOffsetChar( 5 ), gbOffsetLine( 8 ) );
				mfMenuSetTableLabel( st_menu, 2, "Preference" );
				mfMenuSetTableEntry( st_menu, 2, 1, 1, "Buttons to run/stop the macro", macro_hotkey_control, &st_current_slot, pref_hotkey );
				mfMenuSetTableEntry( st_menu, 2, 2, 1, "Number of loop", macro_loop_control, &st_current_slot, pref_loop );
				
				mfMenuSetTablePosition( st_menu, 3, gbOffsetChar( 5 ), gbOffsetLine( 12 ) );
				mfMenuSetTableLabel( st_menu, 3, "Control" );
				mfMenuSetTableEntry( st_menu, 3, 1, 1, "Run", mfCtrlDefCallback, macro_menu_run, NULL );
				mfMenuSetTableEntry( st_menu, 3, 2, 1, "Stop", mfCtrlDefCallback, macro_menu_stop, NULL );
				mfMenuSetTableEntry( st_menu, 3, 3, 1, "Edit", mfCtrlDefCallback, macro_menu_edit, NULL );
				mfMenuSetTableEntry( st_menu, 3, 4, 1, "Clear", mfCtrlDefCallback, macro_menu_clear, NULL );
				
				mfMenuSetTablePosition( st_menu, 4, gbOffsetChar( 5 ), gbOffsetLine( 18 ) );
				mfMenuSetTableLabel( st_menu, 4, "Realtime recording" );
				mfMenuSetTableEntry( st_menu, 4, 1, 1, "Start", mfCtrlDefCallback, macro_menu_record_start, NULL );
				mfMenuSetTableEntry( st_menu, 4, 2, 1, "Append", mfCtrlDefCallback, macro_menu_append_start, NULL );
				mfMenuSetTableEntry( st_menu, 4, 3, 1, "Stop", mfCtrlDefCallback, macro_menu_record_stop, NULL );
				mfMenuSetTableEntry( st_menu, 4, 4, 1, "Recording analog stick movement",macro_analogstick_control, &st_current_slot, NULL );
				
				mfMenuSetTablePosition( st_menu, 5, gbOffsetChar( 5 ), gbOffsetLine( 24 ) );
				mfMenuSetTableLabel( st_menu, 5, "File" );
				mfMenuSetTableEntry( st_menu, 5, 1, 1, "Load", mfCtrlDefExtra, macro_menu_load, NULL );
				mfMenuSetTableEntry( st_menu, 5, 2, 1, "Save", mfCtrlDefExtra, macro_menu_save, NULL );
			}
			mfMenuInitTables( st_menu, 5 );
			macro_select( st_current_slot );
			break;
		case MF_MM_TERM:
			mfMenuDestroyTables( st_menu );
			mfHeapDestroy( st_heap );
			return;
		default:
			gbPrint( gbOffsetChar( 3 ), gbOffsetLine(  4 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "Please choose a operation." );
			if( ! mfMenuDrawTables( st_menu, 5, MF_MENU_NO_OPTIONS ) ) mfMenuProc( NULL );
	}
}

/*-----------------------------------------------
	マクロスロット用メニューコントロール
-----------------------------------------------*/
static void macro_select( unsigned short slot )
{
	unsigned short i;
	bool busy = false;
	
	for( i = 0; i < MACRO_MAX_SLOT; i++ ){
		if( st_slot[i].mode != MACRO_NTD ){
			busy = true;
			break;
		}
	}
	
	mfMenuInactiveTableEntry( st_menu, 3, 1, 1 );
	mfMenuInactiveTableEntry( st_menu, 3, 2, 1 );
	mfMenuInactiveTableEntry( st_menu, 3, 4, 1 );
	
	mfMenuInactiveTableEntry( st_menu, 4, 1, 1 );
	mfMenuInactiveTableEntry( st_menu, 4, 2, 1 );
	mfMenuInactiveTableEntry( st_menu, 4, 3, 1 );
	
	mfMenuInactiveTableEntry( st_menu, 5, 1, 1 );
	mfMenuInactiveTableEntry( st_menu, 5, 2, 1 );
	
	if( st_slot[slot].mode == MACRO_RECORD ){
		mfMenuActiveTableEntry( st_menu, 4, 3, 1 );
		mfMenuInactiveTableEntry( st_menu, 3, 3, 1 );
	} else if( st_slot[slot].mode == MACRO_TRACE ){
		mfMenuInactiveTableEntry( st_menu, 2, 2, 1 );
		mfMenuActiveTableEntry( st_menu, 3, 2, 1 );
		mfMenuInactiveTableEntry( st_menu, 3, 3, 1 );
	} else if( ! busy ){
		if( st_slot[slot].macro ){
			mfMenuActiveTableEntry( st_menu, 3, 1, 1 );
			mfMenuActiveTableEntry( st_menu, 4, 2, 1 );
			mfMenuActiveTableEntry( st_menu, 3, 4, 1 );
			mfMenuActiveTableEntry( st_menu, 5, 2, 1 );
		}
		mfMenuActiveTableEntry( st_menu, 2, 2, 1 );
		mfMenuActiveTableEntry( st_menu, 4, 1, 1 );
		mfMenuActiveTableEntry( st_menu, 3, 3, 1 );
		mfMenuActiveTableEntry( st_menu, 5, 1, 1 );
	} else{
		if( st_slot[slot].macro ){
			mfMenuActiveTableEntry( st_menu, 3, 4, 1 );
		}
	}
}

static bool macro_slot_control( MfMessage message, const char *label, void *var, void *pref, void *ex )
{
	unsigned short slot_num_prev = *((unsigned short *)var);
	struct macro_params *params  = &(st_slot[*((unsigned short *)var)]);
	
	switch( message ){
		case MF_CM_LABEL:
			{
				char *state;
				switch( params->mode ){
					case MACRO_RECORD:
						state = "(Recording)";
						break;
					case MACRO_TRACE:
						state = "(Running)";
						break;
					default:
						if( params->macro ){
							state = "(Available)";
						} else{
							state = "";
						}
				}
				mfCtrlSetLabel( ex, "%s: %d [%s] %s", label, *((unsigned short *)var) + 1, params->name, state );
			}
			break;
		case MF_CM_INFO:
			mfMenuSetInfoText( MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(\x87)Prev, (\x85)Next, (\x84)Edit name" );
			break;
		case MF_CM_PROC:
			if( mfMenuIsPressed( PSP_CTRL_TRIANGLE ) ){
				mfDialogSoskInit( "Macro name", params->name, MACRO_NAME_LENGTH, CDIALOG_SOSK_INPUTTYPE_ASCII );
			} else{
				mfCtrlDefGetNumber( message, label, var, pref, ex );
			}
			break;
		default:
			mfCtrlDefGetNumber( message, label, var, pref, ex );
			break;
	}
	
	if( slot_num_prev != *((unsigned short *)var) ) macro_select( *((unsigned short *)var) );
	
	return true;
}

static bool macro_hotkey_control( MfMessage message, const char *label, void *var, void *pref, void *ex )
{
	return mfCtrlDefGetButtons( message, label, &(st_slot[*((unsigned short *)var)].hotkey), pref, ex );
}

static bool macro_loop_control( MfMessage message, const char *label, void *var, void *pref, void *ex )
{
	if( message == MF_CM_LABEL && st_slot[*((unsigned short *)var)].loopNum == 0 ){
		mfCtrlSetLabel( ex, "%s: 0 (Infinity)", label );
		return true;
	} else{
		return mfCtrlDefGetNumber( message, label, &(st_slot[*((unsigned short *)var)].loopNum), pref, ex );
	}
}

static bool macro_analogstick_control( MfMessage message, const char *label, void *var, void *pref, void *ex )
{
	return mfCtrlDefBool( message, label, &(st_slot[*((unsigned short *)var)].analogStickEnable), pref, ex );
}

/*-----------------------------------------------
	メニュー項目
-----------------------------------------------*/
static void macro_menu_record_start( MfMenuMessage message )
{
	if( message == MF_MM_PROC ){
		if( ! mfIsEnabled() ){
			macro_mfengine_warning();
		} else{
			struct macro_params *params = &(st_slot[st_current_slot]);
			MacromgrCommand *cmd;
			
			if( ! params->macro ){
				params->macro = macromgrNew();
			} else{
				macromgrClear( params->macro );
			}
			
			cmd = macromgrCreateRoot( params->macro );
			macro_record_prepare( params, cmd );
			
			mfMenuQuit();
		}
	}
}

static void macro_menu_append_start( MfMenuMessage message )
{
	if( message == MF_MM_PROC ){
		if( ! mfIsEnabled() ){
			macro_mfengine_warning();
		} else{
			struct macro_params *params = &(st_slot[st_current_slot]);
			MacromgrCommand *cmd = macromgrSeek( params->macro, 0, MACROMGR_SEEK_END, NULL );
			
			cmd = macromgrInsertAfter( params->macro, cmd );
			macro_record_prepare( params, cmd );
			
			mfMenuQuit();
		}
	}
}

static void macro_menu_record_stop( MfMenuMessage message )
{
	if( message == MF_MM_PROC ){
		struct macro_params *params = &(st_slot[st_current_slot]);
		
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "Stopping macro recording..." );
		
		macro_record_stop( params );
		macro_select( st_current_slot );
		
		mfMenuWait( MF_MESSAGE_DELAY );
		mfMenuProc( NULL );
	}
}

static void macro_menu_run( MfMenuMessage message )
{
	if( message == MF_MM_PROC ){
		if( ! mfIsEnabled() ){
			macro_mfengine_warning();
		} else{
			macro_run( &(st_slot[st_current_slot]) );
			mfMenuQuit();
		}
	}
}

static void macro_menu_stop( MfMenuMessage message )
{
	if( message == MF_MM_PROC ){
		struct macro_params *params = &(st_slot[st_current_slot]);
		
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "Macro halted." );
		
		macro_trace_stop( params );
		macro_select( st_current_slot );
		
		mfMenuWait( MF_MESSAGE_DELAY );
		mfMenuProc( NULL );
	}
}

static void macro_menu_edit( MfMenuMessage message )
{
	static bool macroeditor_stat;
	
	if( message == MF_MM_INIT ){
		if( ! st_slot[st_current_slot].macro ){
			dbgprint( "Creating minimize macro" );
			st_slot[st_current_slot].macro = macromgrNew();
			macromgrCreateRoot( st_slot[st_current_slot].macro );
		}
		dbgprint( "Init macroeditor" );
		macroeditor_stat = macroeditorInit( st_slot[st_current_slot].macro );
	} else if( message == MF_MM_PROC ){
		if( macroeditor_stat ){
			if( ! macroeditorMain() ) mfMenuProc( NULL );
		} else{
			mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "Failed to start MacroEditor: Not enough memory" );
			mfMenuWait( MF_ERROR_DELAY );
			mfMenuProc( NULL );
		}
	} else if( message == MF_MM_TERM ){
		dbgprint( "Term macroeditor" );
		macroeditorTerm();
		macro_select( st_current_slot );
	}
}

static void macro_menu_clear( MfMenuMessage message )
{
	if( message == MF_MM_PROC ){
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "Clearing macro..." );
		
		macro_clear( &(st_slot[st_current_slot]) );
		macro_select( st_current_slot );
		
		mfMenuWait( MF_MESSAGE_DELAY );
		mfMenuProc( NULL );
	}
}

static void macro_menu_save( MfMenuMessage message )
{
	static char *path;
	static char *name;
	
	if( message == MF_MM_INIT ){
		path = mfHeapAlloc( st_heap, MF_PATH_MAX );
		name = mfHeapAlloc( st_heap, MACRO_NAME_LENGTH + 4 );
		
		if( (st_slot[st_current_slot].name)[0] != '\0' ){
			snprintf( name, MACRO_NAME_LENGTH + 4, "%s.ini", st_slot[st_current_slot].name );
		} else{
			strutilCopy( name, "macro.ini", MACRO_NAME_LENGTH + 4 );
		}
		
		if( ! mfDialogGetfilenameInit( "Save macro", "ms0:", name, path, 255, CDIALOG_GETFILENAME_SAVE | CDIALOG_GETFILENAME_OVERWRITEPROMPT  ) ){
			mfHeapFree( st_heap, path );
			path = NULL;
			mfMenuExitExtra();
		}
	} else if( message == MF_MM_PROC ){
		if( mfDialogCurrentType() ){
			if( ! mfDialogGetfilenameDraw() ){
				if( ! mfDialogGetfilenameResult() ){
					mfHeapFree( st_heap, path );
					mfHeapFree( st_heap, name );
					path = NULL;
					name = NULL;
				} else{
					mfMenuSetInfoText( MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "Saving %s...", path );
				}
			}
		} else if( path ){
			macro_save( &(st_slot[st_current_slot]), path );
			mfHeapFree( st_heap, path );
			mfHeapFree( st_heap, name );
			path = NULL;
			name = NULL;
		} else if( ! path ){
			mfMenuExitExtra();
		}
	}
}

static void macro_menu_load( MfMenuMessage message )
{
	static char *path;
	
	if( message == MF_MM_INIT ){
		path = mfHeapAlloc( st_heap, MF_PATH_MAX );
		
		if( ! mfDialogGetfilenameInit( "Load macro", "ms0:", "", path, 255, CDIALOG_GETFILENAME_OPEN | CDIALOG_GETFILENAME_FILEMUSTEXIST ) ){
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
				} else{
					mfMenuSetInfoText( MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "Loading %s...", path );
				}
			}
		} else if( path ){
			macro_clear( &(st_slot[st_current_slot]) );
			if( ! macro_load( &(st_slot[st_current_slot]), path ) ){
				macro_clear( &(st_slot[st_current_slot]) );
			}
			mfHeapFree( st_heap, path );
			path = NULL;
		} else if( ! path ){
			macro_select( st_current_slot );
			mfMenuExitExtra();
		}
	}
}

/*-----------------------------------------------
	マクロの操作
-----------------------------------------------*/
static void macro_clear( struct macro_params *slot )
{
	if( slot->macro ){
		macromgrDestroy( slot->macro );
		memset( slot, 0, sizeof( struct macro_params ) );
	}
}

static bool macro_save( struct macro_params *slot, const char *path )
{
	IniUID ini = inimgrNew();
	if( ini ){
		int ret;
		char hotkey[MACRO_HOTKEY_LENGTH];
		
		if( mfConvertButtonReady() ){
			mfConvertButtonC2N( slot->hotkey, hotkey, sizeof( hotkey ) );
			mfConvertButtonFinish();
		} else{
			mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "Cannot save Hotkey: Not enough memory." );
			mfMenuWait( MF_ERROR_DELAY );
		}
		
		inimgrAddSection( ini, MACRO_SECTION_INFO );
		inimgrSetString( ini, MACRO_SECTION_INFO, MACRO_ENTRY_NAME, slot->name );
		inimgrSetString( ini, MACRO_SECTION_INFO, MACRO_ENTRY_HOTKEY, hotkey );
		inimgrSetInt( ini, MACRO_SECTION_INFO, MACRO_ENTRY_REPEAT, slot->loopNum );
		inimgrAddSection( ini, MACRO_SECTION_DATA );
		inimgrSetCallback( ini, MACRO_SECTION_DATA, macromgrSaver, &(slot->macro) );
		ret = inimgrSave( ini, path, MACRO_INI_SIGNATURE, MACROMGR_MACRO_VERSION, 0, 0 );
		inimgrDestroy( ini );
		
		if( ret != 0 ){
			mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "inimgr Error: %X", ret );
			mfMenuWait( MF_ERROR_DELAY );
		} else{
			return true;
		}
	} else{
		mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "inimgr Error: Not enough memory." );
		mfMenuWait( MF_ERROR_DELAY );
	}
	return false;
}

static bool macro_load( struct macro_params *slot, const char *path )
{
	IniUID ini = inimgrNew();
	if( ini ){
		int ret;
		
		if( ! slot->macro ){
			slot->macro = macromgrNew();
			if( ! slot->macro ){
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "macromgr Error: Not enough memory." );
				mfMenuWait( MF_ERROR_DELAY );
				return false;
			}
			inimgrSetCallback( ini, MACRO_SECTION_DATA, macromgrLoader, &(slot->macro) );
		} else{
			inimgrSetCallback( ini, MACRO_SECTION_DATA, macromgrAppendLoader, &(slot->macro) );
		}
		
		ret = inimgrLoad( ini, path, MACRO_INI_SIGNATURE, MACROMGR_MACRO_VERSION, 0, 0 );
		
		if( ret != 0 ){
			macro_clear( slot );
			if( ret == INIMGR_ERROR_INVALID_SIGNATURE ){
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "Failure: Not a MacroFire macro file." );
			} else if( ret == INIMGR_ERROR_INVALID_VERSION ){
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "Failure: Unsupported file version." );
			} else {
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "inimgr Error: %X", ret );
			}
			mfMenuWait( MF_ERROR_DELAY );
		} else{
			char hotkey[MACRO_HOTKEY_LENGTH];
			
			inimgrGetString( ini, MACRO_SECTION_INFO, MACRO_ENTRY_NAME, slot->name, MACRO_NAME_LENGTH );
			inimgrGetString( ini, MACRO_SECTION_INFO, MACRO_ENTRY_HOTKEY, hotkey, sizeof( hotkey ) );
			inimgrGetInt( ini, MACRO_SECTION_INFO, MACRO_ENTRY_REPEAT, (int *)&(slot->loopNum) );
			inimgrDestroy( ini );
			
			if( macromgrSeek( slot->macro, 0, MACROMGR_SEEK_SET, NULL ) == NULL ){
				macromgrDestroy( slot->macro );
				slot->macro = 0;
			}
			
			if( mfConvertButtonReady() ){
				slot->hotkey = mfConvertButtonN2C( hotkey );
				mfConvertButtonFinish();
			} else{
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "Cannot load Hotkey: Not enough memory." );
				mfMenuWait( MF_ERROR_DELAY );
			}
			return true;
		}
		inimgrDestroy( ini );
	} else{
		mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "inimgr Error: Not enough memory." );
		mfMenuWait( MF_ERROR_DELAY );
	}
	return false;
}

/*-----------------------------------------------
	マクロの記録と実行
-----------------------------------------------*/
static void macro_run( struct macro_params *params )
{
	MacromgrCommand *cmd = macromgrSeek( params->macro, 0, MACROMGR_SEEK_SET, NULL );
	
	params->temp.trace.loopRest = params->loopNum;
	macro_trace_prepare( params, cmd );
}

static bool macro_loop( struct macro_params *params )
{
	MacromgrCommand *cmd = macromgrSeek( params->macro, 0, MACROMGR_SEEK_SET, NULL );
	
	if( ! params->temp.trace.loopRest || ( params->temp.trace.loopRest > 1 ) ){
		if( params->temp.trace.loopRest ) params->temp.trace.loopRest--;
		macro_trace_loop( params, cmd );
		return true;
	} else{
		return false;
	}
}

static unsigned int macro_calc_delayms( MacromgrData base )
{
	MacromgrData rtc, waitms;
	sceRtcGetCurrentTick( &rtc );
	
	waitms = ( rtc - base ) / 1000;
	return waitms > MACROMGR_MAX_DELAY ? MACROMGR_MAX_DELAY : waitms;
}

static void macro_record_prepare( struct macro_params *params, MacromgrCommand *start )
{
	params->temp.common.lastButtons = 0;
	params->temp.common.analogX     = MACROMGR_ANALOG_CENTER;
	params->temp.common.analogY     = MACROMGR_ANALOG_CENTER;
	sceRtcGetCurrentTick( &(params->temp.common.rtc) );
	
	params->mode     = MACRO_RECORD;
	params->temp.cmd = start;
}

static void macro_record_stop( struct macro_params *params )
{
	if( params->mode != MACRO_RECORD ) return;
	
	params->mode            = MACRO_NTD;
	params->temp.cmd        = NULL;
	params->temp.common.rtc = 0;
}

static void macro_record( struct macro_params *params, SceCtrlData *pad )
{
	/* 記録開始前にmacro_record_prepare()を呼んで準備すること */
	
	unsigned int current_buttons = pad->Buttons & MF_TARGET_BUTTONS;
	bool         analog_move     = false;
	
	
	if( params->analogStickEnable && ( params->temp.common.analogX != pad->Lx || params->temp.common.analogY != pad->Ly ) ){
		analog_move = true;
	}
	
	if( current_buttons == params->temp.common.lastButtons && ! analog_move ){
		/* ボタンもアナログスティックも前回と同じならばディレイをセットして終了 */
		if( params->temp.cmd->prev ) macromgrSetDelay( params->temp.cmd, macro_calc_delayms( params->temp.common.rtc ) );
	} else{
		/* ボタンかアナログスティックに動きがあった場合 */
		MacromgrAction action;
		MacromgrData data;
		MacromgrData sub;
		
		unsigned int press_buttons   = ( params->temp.common.lastButtons ^ current_buttons ) & current_buttons;
		unsigned int release_buttons = ( params->temp.common.lastButtons ^ current_buttons ) & params->temp.common.lastButtons;
		
		macromgrGetCommand( params->temp.cmd, &action, &data, &sub );
		
		if( action == MACROMGR_DELAY ){
			if( data > 0 ){
				/* 現在のコマンドがディレイで時間が0より大きい場合は、ディレイを更新して次のコマンド作成 */
				macromgrSetDelay( params->temp.cmd, macro_calc_delayms( params->temp.common.rtc ) );
				params->temp.cmd = macromgrInsertAfter( params->macro, params->temp.cmd );
			}
		} else{
			/* 現在のコマンドがディレイじゃない場合は次のコマンド作成 */
			params->temp.cmd = macromgrInsertAfter( params->macro, params->temp.cmd );
		}
		
		/* コマンドの追加に失敗していれば記録終了 */
		if( ! params->temp.cmd ) goto ABORT;
		
		/* ボタンが異なっていればボタン変更コマンドをセット */
		if( press_buttons && release_buttons ){
			macromgrSetButtonsChange( params->temp.cmd, current_buttons );
			params->temp.cmd = macromgrInsertAfter( params->macro, params->temp.cmd );
		} else if( press_buttons ){
			macromgrSetButtonsPress( params->temp.cmd, press_buttons );
			params->temp.cmd = macromgrInsertAfter( params->macro, params->temp.cmd );
		} else if( release_buttons ){
			macromgrSetButtonsRelease( params->temp.cmd, release_buttons );
			params->temp.cmd = macromgrInsertAfter( params->macro, params->temp.cmd );
		}
		
		/* コマンドの追加に失敗していれば記録終了 */
		if( ! params->temp.cmd ) goto ABORT;
		
		/* アナログスティックが動いていれば移動コマンドをセット */
		if( analog_move ){
			macromgrSetAnalogMove( params->temp.cmd, pad->Lx, pad->Ly );
			params->temp.cmd = macromgrInsertAfter( params->macro, params->temp.cmd );
			
			/* コマンドの追加に失敗していれば記録終了 */
			if( ! params->temp.cmd ) goto ABORT;
		}
		
		/* 一時データを更新する */
		params->temp.common.lastButtons = current_buttons;
		params->temp.common.analogX     = pad->Lx;
		params->temp.common.analogY     = pad->Ly;
		sceRtcGetCurrentTick( &(params->temp.common.rtc) );
	}
	
	return;
	
	ABORT:
		macro_record_stop( params );
		return;
}

static void macro_trace_prepare( struct macro_params *params, MacromgrCommand *start )
{
	if( params->mode == MACRO_TRACE ) macro_trace_stop( params );
	
	params->temp.common.lastButtons = 0;
	params->temp.common.analogX     = MACROMGR_ANALOG_CENTER;
	params->temp.common.analogY     = MACROMGR_ANALOG_CENTER;
	
	params->temp.trace.rfUid    = mfRapidfireNew();
	if( ! params->temp.trace.rfUid ) return;
	
	params->mode     = MACRO_TRACE;
	params->temp.cmd = start;
	sceRtcGetCurrentTick( &(params->temp.common.rtc) );
}

static void macro_trace_loop( struct macro_params *params, MacromgrCommand *start )
{
	params->temp.common.lastButtons = 0;
	params->temp.common.analogX     = MACROMGR_ANALOG_CENTER;
	params->temp.common.analogY     = MACROMGR_ANALOG_CENTER;
	params->temp.cmd                = start;
}


static void macro_trace_stop( struct macro_params *params )
{
	if( params->mode != MACRO_TRACE ) return;
	
	mfRapidfireDestroy( params->temp.trace.rfUid );
	params->temp.trace.rfUid = 0;
	
	params->mode            = MACRO_NTD;
	params->temp.cmd        = NULL;
	params->temp.common.rtc = 0;
}

static bool macro_trace( struct macro_params *params, SceCtrlData *pad )
{
	/* 記録開始前にmacro_trace_prepare()を呼んで準備すること */
	MacromgrAction action;
	MacromgrData   data;
	MacromgrData   sub;
	
	do {
		macromgrGetCommand( params->temp.cmd, &action, &data, &sub );
		if( ! data ) continue;
		
		switch( action ){
			case MACROMGR_DELAY: {
					uint64_t current_tick;
					sceRtcGetCurrentTick( &current_tick );
					
					if( ( ( current_tick - params->temp.common.rtc ) / 1000 ) >= data ){
						sceRtcGetCurrentTick( &(params->temp.common.rtc) );
						continue;
					}
				}
				break;
			case MACROMGR_BUTTONS_PRESS:
				if( ! data ) continue;
				params->temp.common.lastButtons |= data;
				break;
			case MACROMGR_BUTTONS_RELEASE:
				if( ! data ) continue;
				params->temp.common.lastButtons ^= data;
				break;
			case MACROMGR_BUTTONS_CHANGE:
				if( ! data ) continue;
				params->temp.common.lastButtons = data;
				break;
			case MACROMGR_ANALOG_MOVE:
				params->temp.common.analogX = MACROMGR_GET_ANALOG_X( data );
				params->temp.common.analogY = MACROMGR_GET_ANALOG_Y( data );
				break;
			case MACROMGR_RAPIDFIRE_START:
				if( ! data ) continue;
				mfRapidfireSetRapid(
					params->temp.trace.rfUid,
					data,
					MACROMGR_GET_RAPIDPDELAY( sub ),
					MACROMGR_GET_RAPIDRDELAY( sub ),
					true
				);
				break;
			case MACROMGR_RAPIDFIRE_STOP:
				if( ! data ) continue;
				mfRapidfireClear( params->temp.trace.rfUid, data );
				break;
			default:
				continue;
		}
		
		mfRapidfireExec( params->temp.trace.rfUid, MF_UPDATE, pad );
		
		/* ボタンを更新 */
		pad->Buttons |= params->temp.common.lastButtons;
		
		/* 最後にマクロで指定されたアナログスティックの座標が実座標よりも動いていれば上書き */
		if( abs( params->temp.common.analogX - MACROMGR_ANALOG_CENTER ) > abs( pad->Lx - MACROMGR_ANALOG_CENTER ) )
			pad->Lx = params->temp.common.analogX;
		
		if( abs( params->temp.common.analogY - MACROMGR_ANALOG_CENTER ) > abs( pad->Ly - MACROMGR_ANALOG_CENTER ) )
			pad->Ly = params->temp.common.analogY;
		
		/* コマンド実行を一つ完了したので抜ける */
		break;
	} while( ( params->temp.cmd = macromgrNext( params->temp.cmd ) ) );
	
	/* コマンドを次に進める */
	if( params->temp.cmd && action != MACROMGR_DELAY ){
		params->temp.cmd = macromgrNext( params->temp.cmd );
		sceRtcGetCurrentTick( &(params->temp.common.rtc) );
	}
	
	/* 全コマンド実行終了したかどうか */
	if( params->temp.cmd ){
		return true;
	} else{
		sceRtcGetCurrentTick( &(params->temp.common.rtc) );
		return false;
	}
}

static void macro_mfengine_warning( void )
{
	gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "MacroFire Engine is currently off." );
	mfMenuWait( MF_MESSAGE_DELAY );
	mfMenuProc( NULL );
}

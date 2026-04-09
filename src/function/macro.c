/*
	Macro
*/

#include "macro.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static MacroData *macro_append             ( MacroData *macro );
static void      macro_record              ( MfCallMode mode, SceCtrlData *pad_data, void *argp );
static bool      macro_trace               ( MfCallMode mode, SceCtrlData *pad_data, void *argp );
static bool      macro_is_busy             ( void );
static bool      macro_is_avail            ( void );
static bool      macro_is_enabled_mf_engine( void );
static void      macro_save                ( FilehUID fuid, char *buf, size_t max );
static void      macro_load                ( IniUID ini, FilehUID fuid, char *buf, size_t max );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static MfMenuCallback st_callback;
static int            st_run_loop     = 0;
static MacroRunMode   st_run_mode     = MRM_NONE;
static bool           st_avail_analog = false;
static MfRapidfireUID st_rfuid;

static unsigned int   st_temp_buttons;
static bool           st_temp_analog_move;
static u64            st_temp_analog_coord;
static u64            st_temp_ms;

static MfMenuItem st_menu_table[] = {
	{ MT_CALLBACK, "Run once",     0, &st_callback, { { .pointer = macroRunOnce },      { .pointer = NULL } } },
	{ MT_CALLBACK, "Run infinity", 0, &st_callback, { { .pointer = macroRunInfinity },  { .pointer = NULL } } },
	{ MT_CALLBACK, "Stop macro",   0, &st_callback, { { .pointer = macroRunInterrupt }, { .pointer = NULL } } },
	{ MT_NULL },
	{ MT_CALLBACK, "Start recording", 0, &st_callback, { { .pointer = macroRecordStart }, { .pointer = NULL } } },
	{ MT_CALLBACK, "Stop recording",  0, &st_callback, { { .pointer = macroRecordStop },  { .pointer = NULL } } },
	{ MT_BOOL,     "Recording analog stick", 0, &st_avail_analog, { { .string = "OFF" }, { .string = "ON" }, { NULL } } },
	{ MT_NULL },
	{ MT_CALLBACK, "Edit macro",   0, &st_callback, { { .pointer = macroEdit },   { .pointer = NULL } } },
	{ MT_CALLBACK, "Clear macro",  0, &st_callback, { { .pointer = macroClear },  { .pointer = NULL } } },
	{ MT_NULL },
	{ MT_CALLBACK, "Load from MemoryStick", 0, &st_callback, { { .pointer = macroLoad }, { .pointer = NULL } } },
	{ MT_CALLBACK, "Save to MemoryStick",   0, &st_callback, { { .pointer = macroSave }, { .pointer = NULL } } }
};

/*=============================================*/

void macroInit( void )
{
	return;
}

void macroTerm( void )
{
	macromgrDestroy();
}

void macroIntr( const bool mfengine )
{
	if( ! mfengine && st_run_mode == MRM_TRACE ){
		/* マクロ実行位置を先頭に戻すのみ */
		macro_trace( MF_CALL_READ, NULL, NULL );
	}
}

MfMenuRc macroRunInterrupt( SceCtrlData *pad_data, void *arg )
{
	if( st_run_mode != MRM_TRACE ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Not ran yet." );
	} else{
		st_run_mode = MRM_NONE;
		if( st_rfuid ){
			mfRapidfireDestroy( st_rfuid );
			st_rfuid = 0;
		}
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Macro halted." );
		macro_trace( MF_CALL_READ, NULL, NULL );
	}
	mfMenuWait( 1000000 );
	return MR_BACK;
}

MfMenuRc macroRunOnce( SceCtrlData *pad_data, void *arg )
{
	if( macro_is_busy() || ! macro_is_avail() || ! macro_is_enabled_mf_engine() ) return MR_BACK;
	
	st_run_loop = 1;
	st_run_mode = MRM_TRACE;
	
	/* 最後のボタン情報をリセット */
	st_temp_buttons      = 0;
	st_temp_analog_move  = false;
	st_temp_analog_coord = MACROMGR_ANALOG_NEUTRAL;
	st_temp_ms           = 0;
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Run once..." );
	mfMenuWait( 1000000 );
	mfMenuQuit();
	
	return MR_BACK;
}

MfMenuRc macroRunInfinity( SceCtrlData *pad_data, void *arg )
{
	if( macro_is_busy() || ! macro_is_avail() || ! macro_is_enabled_mf_engine() ) return MR_BACK;
	
	st_run_loop = -1; /* 無限 */
	st_run_mode = MRM_TRACE;
	
	/* 最後のボタン情報をリセット */
	st_temp_buttons      = 0;
	st_temp_analog_move  = false;
	st_temp_analog_coord = MACROMGR_ANALOG_NEUTRAL;
	st_temp_ms           = 0;
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Run infinity..." );
	mfMenuWait( 1000000 );
	mfMenuQuit();
	
	return MR_BACK;
}

void macroMain( MfCallMode mode, SceCtrlData *pad_data, void *argp )
{
	switch( st_run_mode ){
		case MRM_NONE: break;
		case MRM_TRACE:
			while( macro_trace( mode, pad_data, argp ) );
			break;
		case MRM_RECORD:
			macro_record( mode, pad_data, argp );
			break;
	}
}

MfMenuRc macroLoad( SceCtrlData *pad_data, void *arg )
{
	if( macro_is_busy() ) return MR_BACK;
	
	if( ! mfMenuGetFilenameIsReady() ){
		MfMenuRc rc = mfMenuGetFilenameInit(
			"Open macro",
			CGF_OPEN | CGF_FILEMUSTEXIST,
			"ms0:/"
		);
		if( rc != MR_ENTER ) return MR_BACK;
		
		return MR_CONTINUE;
	} else{
		IniUID ini;
		char *inipath, *path, *name;
		MfMenuRc rc = mfMenuGetFilename( &path, &name );
		
		if( rc != MR_ENTER ) return rc;
		
		inipath = (char *)memsceMalloc( 256 );
		if( ! inipath ){
			mfMenuGetFilenameFree();
			return MR_BACK;
		} else{
			sprintf( inipath, "%s%s", path, name );
			mfMenuGetFilenameFree();
		}
		
		ini = inimgrNew();
		if( ini > 0 ){
			unsigned int err;
			inimgrSetCallback( ini, "CommandSequence", macro_load, NULL );
			if( ( err = inimgrLoad( ini, inipath ) ) == 0 ){
				int version = inimgrGetInt( ini, "default", MACRO_DATA_SIGNATURE, 0 );
				if( version <= 1 ){
					blitString( blitOffsetChar( 3 ), blitOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FCCOLOR, "Macro file is invalid or too lower version." );
					mfMenuWait( MFM_DISPLAY_MICROSEC_ERROR );
				} else{
					blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FGCOLOR, "Loaded from %s.", inipath );
					mfMenuWait( MFM_DISPLAY_MICROSEC_INFO );
				}
			} else{
				blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FCCOLOR, "Failed to load from %s: 0x%X", inipath, err );
				mfMenuWait( MFM_DISPLAY_MICROSEC_ERROR );
			}
			inimgrDestroy( ini );
		}
		memsceFree( inipath );
		
		return MR_BACK;
	}
}

MfMenuRc macroSave( SceCtrlData *pad_data, void *arg )
{
	if( macro_is_busy() || ! macro_is_avail() ) return MR_BACK;
	
	if( ! mfMenuGetFilenameIsReady() ){
		MfMenuRc rc = mfMenuGetFilenameInit(
			"Save macro",
			CGF_SAVE | CGF_FILEMUSTEXIST,
			"ms0:/"
		);
		if( rc != MR_ENTER ) return MR_BACK;
		
		return MR_CONTINUE;
	} else{
		IniUID ini;
		char *inipath, *path, *name;
		MfMenuRc rc = mfMenuGetFilename( &path, &name );
		
		if( rc != MR_ENTER ) return rc;
		
		inipath = (char *)memsceMalloc( 256 );
		if( ! inipath ){
			mfMenuGetFilenameFree();
			return MR_BACK;
		} else{
			sprintf( inipath, "%s%s", path, name );
			mfMenuGetFilenameFree();
		}
		
		ini = inimgrNew();
		if( ini > 0 ){
			unsigned int err;
			inimgrSetInt( ini, "default", MACRO_DATA_SIGNATURE, MACRO_DATA_VERSION );
			inimgrAddSection(  ini, "CommandSequence" );
			inimgrSetCallback( ini, "CommandSequence", NULL, macro_save );
			if( ( err = inimgrSave( ini, inipath ) ) == 0 ){
				blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FGCOLOR, "Saved to %s.", inipath );
				mfMenuWait( MFM_DISPLAY_MICROSEC_INFO );
			} else{
				blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FCCOLOR, "Failed to save to %s: 0x%X", inipath, err );
				mfMenuWait( MFM_DISPLAY_MICROSEC_ERROR );
			}
			inimgrDestroy( ini );
		}
		memsceFree( inipath );
		
		return MR_BACK;
	}
}

MfMenuRc macroEdit( SceCtrlData *pad_data, void *arg )
{
	if( macro_is_busy() ) return MR_BACK;
	
	if( ! macromgrGetCount() ){
			MacroData *macro = macromgrNew();
		if( ! macro ){
			blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Failed to create." );
			mfMenuWait( 1000000 );
			return MR_BACK;
		}
		
		macromgrCmdInit( macro, MA_DELAY, 0, 0 );
		
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Creating new macro..." );
		mfMenuWait( 1000000 );
		return MR_CONTINUE;
	}
	
	return macroeditorMain( pad_data, macromgrGetFirst() );
}

MfMenuRc macroClear( SceCtrlData *pad_data, void *arg )
{
	if( macro_is_busy() || ! macro_is_avail() ) return MR_BACK;
	
	macromgrDestroy();
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Clearing currently macro..." );
	mfMenuWait( 1000000 );
	
	return MR_BACK;
}

MfMenuRc macroRecordStop( SceCtrlData *pad_data, void *arg )
{
	if( st_run_mode == MRM_RECORD ){
		st_run_mode = MRM_NONE;
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Stopping macro recording..." );
	} else{
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Not recorded yet." );
	}
	
	mfMenuWait( 1000000 );
	return MR_BACK;
}

MfMenuRc macroRecordStart( SceCtrlData *pad_data, void *arg )
{
	if( macro_is_busy() || ! macro_is_enabled_mf_engine() ) return MR_BACK;
	
	if( st_run_mode == MRM_RECORD ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Already started recording" );
		mfMenuWait( 1000000 );
		return MR_BACK;
	}
	
	/* 既にマクロがあればクリア */
	if( macromgrGetCount() ) macromgrDestroy();
	
	/* 最後のボタン情報をリセット */
	st_temp_buttons      = 0;
	st_temp_analog_move  = false;
	st_temp_analog_coord = MACROMGR_ANALOG_NEUTRAL;
	st_temp_ms           = 0;
	
	st_run_mode = MRM_RECORD;
	
	mfMenuQuit();
	
	return MR_BACK;
}

MfMenuRc macroMenu( SceCtrlData *pad_data, void *arg )
{
	static int  selected = 0;
	
	if( ! st_callback.func ){
		switch( mfMenuUniDraw( blitOffsetChar( 5 ), blitOffsetLine( 6 ), st_menu_table, MF_ARRAY_NUM( st_menu_table ), &selected, 0 ) ){
			case MR_CONTINUE:
				blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Please choose a operation." );
				switch( selected ){
					case MACRO_RUN_ONCE     : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Running currently macro for once." ); break;
					case MACRO_RUN_INFINITY : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Running currently macro for endless." ); break;
					case MACRO_RUN_HALT     : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Stopping currently running macro." ); break;
					case MACRO_RECORD_START : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Starting to record a macro." ); break;
					case MACRO_RECORD_STOP  : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Stopping to record a macro." ); break;
					case MACRO_ANALOG_OPTION: blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Recording the analog stick movement." ); break;
					case MACRO_EDIT         : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Editing currently macro." ); break;
					case MACRO_CLEAR        : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Clearing currently macro." ); break;
					case MACRO_LOAD         : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Loading a macro from MemoryStick.\n\nIMPORTANT CAUTION:\n  Verify that MemoryStick access indicator is not blinking.\n  Otherwise, will CRASH the currently running game!" ); break;
					case MACRO_SAVE         : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Saving currently macro to MemoryStick.\n\nIMPORTANT CAUTION:\n  Verify that MemoryStick access indicator is not blinking.\n  Otherwise, will CRASH the currently running game!" ); break;
				}
				break;
			case MR_ENTER:
				break;
			case MR_BACK:
				return MR_BACK;
		}
	} else{
		if( ( (MacroFunction)(st_callback.func) )( pad_data, MFM_GET_CB_ARG_BY_PTR( st_callback.arg, 0 ) ) == MR_BACK ){
			st_callback.func = NULL;
		}
	}
	return MR_CONTINUE;
}

/*-----------------------------------------------
	Static関数
------------------------------------------------*/
static MacroData *macro_append( MacroData *macro )
{
	MacroData *new_macro;
	
	if( macro ){
		new_macro = macromgrInsert( MIP_AFTER, macro );
	} else{
		new_macro = macromgrNew();
	}
	
	return new_macro;
}

static void macro_record( MfCallMode mode, SceCtrlData *pad_data, void *argp )
{
	unsigned int cur_buttons;
	u64 cur_analog_coord = MACROMGR_ANALOG_NEUTRAL;
	static MacroData *cur_macro;
	
	if( mode == MF_CALL_LATCH || ! pad_data ) return;
	
	/* チェックするボタンをマスク */
	cur_buttons = pad_data->Buttons & 0x0000FFFF;
	
	if( st_avail_analog ){
		cur_analog_coord = MACROMGR_SET_ANALOG_XY( pad_data->Lx, pad_data->Ly );
	}
	
	if( ! macromgrGetCount() ) cur_macro = NULL;
	
	if( st_temp_buttons == cur_buttons && cur_analog_coord == st_temp_analog_coord ){
		u64 waitms;
		sceRtcGetCurrentTick( &waitms );
		waitms /= 1000;
		
		if( cur_macro && cur_macro->action == MA_DELAY ){
			u64 diff = ( waitms - st_temp_ms );
			cur_macro->data = diff > MACROMGR_MAX_DELAY ? MACROMGR_MAX_DELAY : diff;
		} else{
			cur_macro = macro_append( cur_macro );
			if( ! cur_macro ) return;
			
			macromgrCmdInit( cur_macro, MA_DELAY, 0, st_temp_buttons );
			st_temp_ms = waitms;
		}
	} else{
		unsigned int press_buttons   = ( st_temp_buttons ^ cur_buttons ) & cur_buttons;
		unsigned int release_buttons = ( st_temp_buttons ^ cur_buttons ) & st_temp_buttons;
		
		if( cur_macro ){
			if( ! ( cur_macro->action == MA_DELAY && cur_macro->data == 0 ) ){
				if( cur_macro->action == MA_DELAY && cur_macro->data != 0 ){
					u64 waitms;
					sceRtcGetCurrentTick( &waitms );
					waitms /= 1000;
					
					u64 diff = ( waitms - st_temp_ms );
					cur_macro->data = diff > MACROMGR_MAX_DELAY ? MACROMGR_MAX_DELAY : diff;
				}
				cur_macro = macro_append( cur_macro );
			}
		} else{
			cur_macro = macro_append( cur_macro );
		}
		
		if( ! cur_macro ) return;
		
		if( cur_macro->action == MA_DELAY && cur_macro->data != 0 ){
			u64 waitms;
			sceRtcGetCurrentTick( &waitms );
			waitms /= 1000;
			
			u64 diff = ( waitms - st_temp_ms );
			cur_macro->data = diff > MACROMGR_MAX_DELAY ? MACROMGR_MAX_DELAY : diff;
		}
		
		if( press_buttons && release_buttons ){
			macromgrCmdInit( cur_macro, MA_BUTTONS_CHANGE, cur_buttons, st_temp_buttons );
		} else if( press_buttons ){
			macromgrCmdInit( cur_macro, MA_BUTTONS_PRESS, press_buttons, st_temp_buttons );
		} else if( release_buttons ){
			macromgrCmdInit( cur_macro, MA_BUTTONS_RELEASE, release_buttons, st_temp_buttons );
		}
		
		if( cur_analog_coord != st_temp_analog_coord ){
			cur_macro = macro_append( cur_macro );
			if( ! cur_macro ) return;
			
			macromgrCmdInit( cur_macro, MA_ANALOG_MOVE, cur_analog_coord, 0 );
		}
		
		st_temp_buttons      = cur_buttons;
		st_temp_analog_coord = cur_analog_coord;
	}
}

static bool macro_trace( MfCallMode mode, SceCtrlData *pad_data, void *argp )
{
	static MacroData *cur_macro = NULL;
	bool redo = false;
	
	if( ! macromgrGetCount() || ! pad_data || st_run_mode != MRM_TRACE ){
		cur_macro = NULL;
		st_run_mode = MRM_NONE;
		return false;
	} else if( mode == MF_CALL_LATCH ){
		pad_data->Buttons |= st_temp_buttons;
		return false;
	}
	
	if( ! cur_macro ){
		if( ! st_run_loop ){
			if( st_rfuid ){
				mfRapidfireDestroy( st_rfuid );
				st_rfuid = 0;
			}
		 	cur_macro = NULL;
			st_run_mode = MRM_NONE;
			return false;
		} else if( st_run_loop < 0 || st_run_loop-- ){
			if( ! st_rfuid ){
				st_rfuid = mfRapidfireNew();
			} else{
				mfClearRapidfireAll( st_rfuid );
			}
			cur_macro = macromgrGetFirst();
		}
	}
	
	if( cur_macro->action == MA_DELAY ){
		u64 waitms;
		sceRtcGetCurrentTick( &waitms );
		waitms /= 1000;
		
		if( ! st_temp_ms ){
			st_temp_ms = waitms;
		} else if( ( waitms - st_temp_ms ) > cur_macro->data ){
			st_temp_ms = 0;
		}
	}
	if( st_temp_ms ){
		redo = false;
	} else if( cur_macro->action == MA_DELAY ){
		cur_macro = cur_macro->next;
		redo = true;
	} else{
		switch( cur_macro->action ){
			case MA_BUTTONS_PRESS:
				st_temp_buttons |= cur_macro->data;
				break;
			case MA_BUTTONS_RELEASE:
				st_temp_buttons ^= cur_macro->data;
				break;
			case MA_BUTTONS_CHANGE:
				st_temp_buttons = cur_macro->data;
				break;
			case MA_ANALOG_MOVE:
				st_temp_analog_move  = true;
				st_temp_analog_coord = cur_macro->data;
				break;
			case MA_RAPIDFIRE_START:
				mfSetRapidfire(
					st_rfuid,
					(unsigned int)(cur_macro->data),
					MF_RAPIDFIRE_MODE_AUTORAPID,
					MACROMGR_GET_RAPIDPDELAY( cur_macro->sub ),
					MACROMGR_GET_RAPIDRDELAY( cur_macro->sub )
				);
				break;
			case MA_RAPIDFIRE_STOP:
				mfClearRapidfire( st_rfuid, (unsigned int)(cur_macro->data) );
				break;
		}
		redo = false;
		cur_macro = cur_macro->next;
	}
	
	mfRapidfire( st_rfuid, mode, pad_data );
	
	pad_data->Buttons |= st_temp_buttons;
	if( st_temp_analog_move ){
		pad_data->Lx = MACROMGR_GET_ANALOG_X( st_temp_analog_coord );
		pad_data->Ly = MACROMGR_GET_ANALOG_Y( st_temp_analog_coord );
	}
	
	return redo;
}

static bool macro_is_busy( void )
{
	if( st_run_mode != MRM_NONE ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Macro function is busy." );
		mfMenuWait( 1000000 );
		return true;
	}
	return false;

}

static bool macro_is_avail( void )
{
	if( ! macromgrGetCount() ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "No current macro." );
		mfMenuWait( 1000000 );
		return false;
	}
	return true;
}

static bool macro_is_enabled_mf_engine( void )
{
	if( mfIsDisabled() ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "MacroFire Engine is currently off." );
		mfMenuWait( 1000000 );
		return false;
	}
	return true;
}

static void macro_load( IniUID ini, FilehUID fuid, char *buf, size_t max )
{
	MacroData *command = NULL;
	char *action, *data;
	
	if( inimgrGetInt( ini, "default", MACRO_DATA_SIGNATURE, 0 ) <= 1 ) return;
	
	if( macromgrGetCount() ) macromgrDestroy();
	
	while( inimgrCBReadln( fuid, buf, max ) ){
		strutilRemoveChar( buf, "\t\x20" );
		
		data = strchr( buf, ':' );
		if( ! data ) continue;
		*data = '\0';
		data++;
		action = buf;
		
		if( strcmp( action, MACRO_ACTION_DELAY ) == 0 ){
			command = macro_append( command );
			macromgrCmdInit( command, MA_DELAY, strtoul( data, NULL, 10 ), 0 );
		} else if( strcmp( action, MACRO_ACTION_BTNPRESS ) == 0 ){
			command = macro_append( command );
			macromgrCmdInit( command, MA_BUTTONS_PRESS, ctrlpadUtilStringToButtons( data ), 0 );
		} else if( strcmp( action, MACRO_ACTION_BTNRELEASE ) == 0 ){
			command = macro_append( command );
			macromgrCmdInit( command, MA_BUTTONS_RELEASE, ctrlpadUtilStringToButtons( data ), 0 );
		} else if( strcmp( action, MACRO_ACTION_BTNCHANGE ) == 0 ){
			command = macro_append( command );
			macromgrCmdInit( command, MA_BUTTONS_CHANGE, ctrlpadUtilStringToButtons( data ), 0 );
		} else if( strcmp( action, MACRO_ACTION_ALMOVE ) == 0 ){
			int x, y;
			char *data2 = strchr( data, ',' );
			if( ! data2 ) continue;
			
			*data2 = '\0';
			data2++;
			
			x = strtol( data, NULL, 10 );
			y = strtol( data2, NULL, 10 );
			
			command = macro_append( command );
			macromgrCmdInit( command, MA_ANALOG_MOVE, MACROMGR_SET_ANALOG_XY( x, y ), 0 );
		} else if( strcmp( action, MACRO_ACTION_BTNRAPIDSTART ) == 0 ){
			char *pdelay, *rdelay, *buttons;
			pdelay = data;
			rdelay = strchr( pdelay, ',' );
			if( ! rdelay ) continue;
			
			*rdelay = '\0';
			rdelay++;
			
			buttons = strchr( rdelay, ',' );
			if( ! buttons ) continue;
			
			*buttons = '\0';
			buttons++;
			
			command = macro_append( command );
			macromgrCmdInit( command, MA_RAPIDFIRE_START, ctrlpadUtilStringToButtons( buttons ), MACROMGR_SET_RAPIDDELAY( strtoul( pdelay, NULL, 10 ), strtoul( rdelay, NULL, 10 ) ) );
		} else if( strcmp( action, MACRO_ACTION_BTNRAPIDSTOP ) == 0 ){
			command = macro_append( command );
			macromgrCmdInit( command, MA_RAPIDFIRE_STOP, ctrlpadUtilStringToButtons( data ), 0 );
		}
	}
}

static void macro_save( FilehUID fuid, char *buf, size_t max )
{
	MacroData *command;
	char buttons[128];
	
	for( command = macromgrGetFirst(); command; command = command->next ){
		switch( command->action ){
			case MA_DELAY:
				snprintf( buf, max, "%s: %llu", MACRO_ACTION_DELAY, (uint64_t)command->data );
				break;
			case MA_BUTTONS_PRESS:
				ctrlpadUtilButtonsToString( command->data, buttons, sizeof( buttons ) );
				snprintf( buf, max, "%s: %s", MACRO_ACTION_BTNPRESS, buttons );
				break;
			case MA_BUTTONS_RELEASE:
				ctrlpadUtilButtonsToString( command->data, buttons, sizeof( buttons ) );
				snprintf( buf, max, "%s: %s", MACRO_ACTION_BTNRELEASE, buttons );
				break;
			case MA_BUTTONS_CHANGE:
				ctrlpadUtilButtonsToString( command->data, buttons, sizeof( buttons ) );
				snprintf( buf, max, "%s: %s", MACRO_ACTION_BTNCHANGE, buttons );
				break;
			case MA_ANALOG_MOVE:
				snprintf( buf, max, "%s: %d,%d", MACRO_ACTION_ALMOVE, (int)MACROMGR_GET_ANALOG_X( command->data ), (int)MACROMGR_GET_ANALOG_Y( command->data ) );
				break;
			case MA_RAPIDFIRE_START:
				ctrlpadUtilButtonsToString( command->data, buttons, sizeof( buttons ) );
				snprintf( buf, max, "%s: %lu,%lu,%s", MACRO_ACTION_BTNRAPIDSTART, MACROMGR_GET_RAPIDPDELAY( command->sub ), MACROMGR_GET_RAPIDRDELAY( command->sub ), buttons );
				break;
			case MA_RAPIDFIRE_STOP:
				ctrlpadUtilButtonsToString( command->data, buttons, sizeof( buttons ) );
				snprintf( buf, max, "%s: %s", MACRO_ACTION_BTNRAPIDSTOP, buttons );
				break;
		}
		inimgrCBWriteln( fuid, buf );
	}
}


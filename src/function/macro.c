/*
	Macro
*/

#include "macro.h"

/*-----------------------------------------------
	ローカル変数と定数
-----------------------------------------------*/
static int          st_runLoop           = 0;
static MacroRunMode st_runMode           = MRM_NONE;
static unsigned int st_temp_buttons      = 0;
static bool         st_temp_analog_move  = false;
static u64          st_temp_analog_coord = 0;
static u64          st_temp_tick         = 0;

#define MACRO_ANALOG_OFF 0
#define MACRO_ANALOG_ON  1
static int st_availAnalog = MACRO_ANALOG_OFF;

#define MACRO_RUN_ONCE      0
#define MACRO_RUN_INFINITY  1
#define MACRO_RUN_HALT      2
#define MACRO_RECORD_START  4
#define MACRO_RECORD_STOP   5
#define MACRO_ANALOG_OPTION 6
#define MACRO_EDIT          8
#define MACRO_CLEAR         9
#define MACRO_CREATE        10
#define MACRO_LOAD          12
#define MACRO_SAVE          13
static MfMenuItem st_macroMenu[] = {
	{ MT_ANCHOR, 0, "Run once",     { 0 } },
	{ MT_ANCHOR, 0, "Run infinity", { 0 } },
	{ MT_ANCHOR, 0, "Stop macro",   { 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_ANCHOR, 0, "Start recording", { 0 } },
	{ MT_ANCHOR, 0, "Stop recording",  { 0 } },
	{ MT_OPTION, &(st_availAnalog), "Recording analog stick", { "OFF", "ON", 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_ANCHOR, 0, "Edit macro", { 0 } },
	{ MT_ANCHOR, 0, "Clear macro",  { 0 } },
	{ MT_ANCHOR, 0, "Create macro", { 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_ANCHOR, 0, "Load from MemoryStick", { 0 } },
	{ MT_ANCHOR, 0, "Save to MemoryStick",   { 0 } }
};

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static MacroData *macro_append             ( MacroData *macro );
static void      macro_record              ( MfCallMode mode, SceCtrlData *pad_data, void *argp );
static void      macro_trace               ( MfCallMode mode, SceCtrlData *pad_data, void *argp );
static bool      macro_is_busy             ( void );
static bool      macro_is_avail            ( void );
static bool      macro_is_enabled_mf_engine( void );

/*=============================================*/

void macroInit( ConfmgrHandler conf[] )
{
	return;
}

void macroTerm( void )
{
	macromgrDestroy();
}
void macroIntr( const int mfengine )
{
	if( mfengine == MF_ENGINE_OFF && st_runMode == MRM_TRACE ){
		/* マクロ実行位置を先頭に戻すのみ */
		macro_trace( MF_CALL_READ, NULL, NULL );
	}
}

MfMenuReturnCode macroRunInterrupt( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( st_runMode != MRM_TRACE ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Not ran yet." );
	} else{
		st_runMode = MRM_NONE;
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Macro halted." );
		macro_trace( MF_CALL_READ, NULL, NULL );
	}
	mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
	return MR_BACK;
}

MfMenuReturnCode macroRunOnce( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( macro_is_busy() || ! macro_is_avail() || ! macro_is_enabled_mf_engine() ) return MR_BACK;
	
	st_runLoop = 1;
	st_runMode = MRM_TRACE;
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Run once..." );
	mfWaitScreenReload( 1 );
	mfMenuQuit();
	
	return MR_BACK;
}

MfMenuReturnCode macroRunInfinity( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( macro_is_busy() || ! macro_is_avail() || ! macro_is_enabled_mf_engine() ) return MR_BACK;
	
	st_runLoop = 0;
	st_runMode = MRM_TRACE;
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Run infinity..." );
	mfWaitScreenReload( 1 );
	mfMenuQuit();
	
	return MR_BACK;
}

void macroMain( MfCallMode mode, SceCtrlData *pad_data, void *argp )
{
	switch( st_runMode ){
		case MRM_NONE: break;
		case MRM_TRACE:  macro_trace( mode, pad_data, argp );  break;
		case MRM_RECORD: macro_record( mode, pad_data, argp ); break;
	}
}

MfMenuReturnCode macroLoad( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	CmndlgOpenFilename cofn;
	
	char dir[256]  = { 0 };
	char name[128] = { 0 };
	char *path;
	
	CmndlgGetFilenameRc cgfrc;
	FilehUID fuid = 0;
	MacroData *cur_macro = NULL;
	char record[MACRO_DATA_RECORD_MAXLEN];
	char *did, *value, *saveptr;
	
	if( macro_is_busy() ) return MR_BACK;
	
	cofn.dirPath     = dir;
	cofn.dirPathMax  = sizeof( dir );
	cofn.fileName    = name;
	cofn.fileNameMax = sizeof( name );
	
	cgfrc = cmndlgGetOpenFilename( "Open a macro.", "ms0:", &cofn );
	
	mfClearColor( MENU_BGCOLOR );
	mfDrawMainFrame();
	
	if( cgfrc == CMNDLG_GETFILENAME_CANCEL ) return MR_BACK;
	
	path = (char *)memsceMalloc( strlen( dir ) + strlen( name ) + 1 );
	if( ! path ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FCCOLOR, MENU_BGCOLOR, "Failed to create loading path." );
		mfWaitScreenReload( MACRO_ERROR_DISPLAY_SEC );
		return MR_BACK;
	}
	sprintf( path, "%s/%s", dir, name );
	
	fuid = filehOpen( path, PSP_O_RDONLY, 0777 );
	if( ! fuid || filehGetLastError( fuid ) < 0 ){
		blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FCCOLOR, MENU_BGCOLOR, "Failed to load %s: %x-%x", path, filehGetLastError( fuid ), filehGetLastSystemError( fuid ) );
		mfWaitScreenReload( MACRO_ERROR_DISPLAY_SEC );
		goto DESTROY;
	}
	
	/* シグネチャを読み込み、バーションをチェック */
	filehReadln( fuid, record, sizeof( record ) );
	strutilToUpper( record );
	did   = strtok_r( record,  MACRO_DATA_RECORD_SEPARATOR, &saveptr );
	value = strtok_r( NULL, MACRO_DATA_RECORD_SEPARATOR, &saveptr );
	if( ! did || ! value || strcmp( did, MACRO_DATA_SIGNATURE ) != 0 ){
		blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FCCOLOR, MENU_BGCOLOR, "The file's signature is invalid." );
		mfWaitScreenReload( MACRO_ERROR_DISPLAY_SEC );
		goto DESTROY;
	} else{
		int ver = strtol( value, NULL, 10 );
		if( ver > MACRO_DATA_VERSION ){
			blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FCCOLOR, MENU_BGCOLOR, "The file's version is not supported." );
			mfWaitScreenReload( MACRO_ERROR_DISPLAY_SEC );
			goto DESTROY;
		}
	}
	
	/* 既にマクロがあればクリア */
	if( macromgrGetCount() ) macromgrDestroy();
	
	blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Loading from %s...", path );
	while( filehReadln( fuid, record, sizeof( record ) ) ){
		strutilToUpper( record );
		did   = strtok_r( record,  MACRO_DATA_RECORD_SEPARATOR, &saveptr );
		value = strtok_r( NULL, MACRO_DATA_RECORD_SEPARATOR, &saveptr );
		
		if( ! did || ! value ) continue;
		
		cur_macro = macro_append( cur_macro );
		
		if( strcmp( did, MACRO_ACTION_DELAY ) == 0 ){
			cur_macro->action = MA_DELAY;
		} else if( strcmp( did, MACRO_ACTION_BTNPRESS ) == 0 ){
			cur_macro->action = MA_BUTTONS_PRESS;
		} else if( strcmp( did, MACRO_ACTION_BTNRELEASE ) == 0 ){
			cur_macro->action = MA_BUTTONS_RELEASE;
		} else if( strcmp( did, MACRO_ACTION_BTNCHANGE ) == 0 ){
			cur_macro->action = MA_BUTTONS_CHANGE;
		} else if( strcmp( did, MACRO_ACTION_ALNEUTRAL ) == 0 ){
			cur_macro->action = MA_ANALOG_NEUTRAL;
		} else if( strcmp( did, MACRO_ACTION_ALMOVE ) == 0 ){
			cur_macro->action = MA_ANALOG_MOVE;
		} else{
			continue;
		}
		
		cur_macro->data = strtoul( value, NULL, 16 );
	}
	
	goto DESTROY;
	
	DESTROY:
		if( fuid ) filehClose( fuid );
		memsceFree( path );
		return MR_BACK;
}

MfMenuReturnCode macroSave( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	CmndlgOpenFilename cofn;
	
	char dir[256]  = { 0 };
	char name[128] = { 0 };
	char *path;
	
	CmndlgGetFilenameRc cgfrc;
	FilehUID fuid = 0;
	MacroData *cur_macro;
	
	if( macro_is_busy() || ! macro_is_avail() ) return MR_BACK;
	
	cofn.dirPath     = dir;
	cofn.dirPathMax  = sizeof( dir );
	cofn.fileName    = name;
	cofn.fileNameMax = sizeof( name );
	
	cgfrc = cmndlgGetSaveFilename( "Save the macro as ...", "ms0:", &cofn );
	
	mfClearColor( MENU_BGCOLOR );
	mfDrawMainFrame();
	
	if( cgfrc == CMNDLG_GETFILENAME_CANCEL ) return MR_BACK;
	
	path = (char *)memsceMalloc( strlen( dir ) + strlen( name ) + 1 );
	if( ! path ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FCCOLOR, MENU_BGCOLOR, "Failed to create saving path." );
		mfWaitScreenReload( MACRO_ERROR_DISPLAY_SEC );
		return MR_BACK;
	}
	sprintf( path, "%s/%s", dir, name );
	
	fuid = filehOpen( path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	if( ! fuid || filehGetLastError( fuid ) < 0 ){
		blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FCCOLOR, MENU_BGCOLOR, "Failed to save %s: %x-%x", path, filehGetLastError( fuid ), filehGetLastSystemError( fuid ) );
		mfWaitScreenReload( MACRO_ERROR_DISPLAY_SEC );
		goto DESTROY;
	}
	
	blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Saving to %s...", path );
	
	/* 先頭にシグネチャを書き込む */
	filehWritef( fuid, MACRO_DATA_RECORD_MAXLEN, "%s%s%d\n\n", MACRO_DATA_SIGNATURE, MACRO_DATA_RECORD_SEPARATOR, MACRO_DATA_VERSION );
	
	cur_macro = macromgrGetFirst();
	for( ; cur_macro; cur_macro = cur_macro->next ){
		switch( cur_macro->action ){
			case MA_DELAY:
				filehWrite( fuid, MACRO_ACTION_DELAY, strlen( MACRO_ACTION_DELAY ) );
				break;
			case MA_BUTTONS_PRESS:
				filehWrite( fuid, MACRO_ACTION_BTNPRESS, strlen( MACRO_ACTION_BTNPRESS ) );
				break;
			case MA_BUTTONS_RELEASE:
				filehWrite( fuid, MACRO_ACTION_BTNRELEASE, strlen( MACRO_ACTION_BTNRELEASE ) );
				break;
			case MA_BUTTONS_CHANGE:
				filehWrite( fuid, MACRO_ACTION_BTNCHANGE, strlen( MACRO_ACTION_BTNCHANGE ) );
				break;
			case MA_ANALOG_NEUTRAL:
				filehWrite( fuid, MACRO_ACTION_ALNEUTRAL, strlen( MACRO_ACTION_ALNEUTRAL ) );
				break;
			case MA_ANALOG_MOVE:
				filehWrite( fuid, MACRO_ACTION_ALMOVE, strlen( MACRO_ACTION_ALMOVE ) );
				break;
		}
		filehWritef( fuid, MACRO_DATA_RECORD_MAXLEN, "%s%lx\n", MACRO_DATA_RECORD_SEPARATOR, cur_macro->data );
	}
	
	goto DESTROY;
	
	DESTROY:
		if( fuid ) filehClose( fuid );
		memsceFree( path );
		return MR_BACK;
}

MfMenuReturnCode macroEdit( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( macro_is_busy() || ! macro_is_avail() ) return MR_BACK;
	
	return macroeditorMain( pad_latch, pad_data, macromgrGetFirst() );
}

MfMenuReturnCode macroClear( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( macro_is_busy() || ! macro_is_avail() ) return MR_BACK;
	
	macromgrDestroy();
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Clearing currently macro..." );
	mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
	
	return MR_BACK;
}

MfMenuReturnCode macroCreate( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	MacroData *macro;
	
	if( macro_is_busy() ) return MR_BACK;
	
	/* 既にマクロがあればクリア */
	if( macromgrGetCount() ) macromgrDestroy();
	
	macro = macromgrNew();
	if( ! macro ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Failed to create." );
		mfWaitScreenReload( MACRO_ERROR_DISPLAY_SEC );
		return MR_BACK;
	}
	
	macro->action = MA_DELAY;
	macro->data   = 0;
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Creating new macro..." );
	mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
	
	return MR_BACK;
}

MfMenuReturnCode macroRecordStop( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( st_runMode == MRM_RECORD ){
		st_runMode = MRM_NONE;
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Stopping macro recording..." );
	} else{
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Not recorded yet." );
	}
	
	mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
	return MR_BACK;
}

MfMenuReturnCode macroRecordStart( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	static int sec = 3;
	
	if( macro_is_busy() || ! macro_is_enabled_mf_engine() ) return MR_BACK;
	
	if( st_runMode == MRM_RECORD ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Already started recording" );
		mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
		return MR_BACK;
	}
	
	/* 既にマクロがあればクリア */
	if( macromgrGetCount() ) macromgrDestroy();
	
	while( sec-- ){
		blitFillBox( 3, 50, 480, 8, MENU_BGCOLOR );
		
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Please RELEASING ALL BUTTONS until returning to the game..." );
		blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 5 ), MENU_FGCOLOR, MENU_BGCOLOR, "%d seconds...", sec + 1 );
		mfWaitScreenReload( 1 );
		
		return MR_CONTINUE;
	}
	
	sec = 3;
	st_runMode = MRM_RECORD;
	
	mfMenuQuit();
	
	return MR_BACK;
}

MfMenuReturnCode macroMenu( SceCtrlLatch *pad_latch, SceCtrlData *pad_data, void *argp )
{
	static int  selected = 0;
	static int  old_selected = 0;
	static MacroFunction function;
	
	if( ! function ){
		switch( mfMenuVertical( blitOffsetChar( 5 ), blitOffsetLine( 4 ), BLIT_SCR_WIDTH, st_macroMenu, MF_ARRAY_NUM( st_macroMenu ), &selected ) ){
			case MR_CONTINUE:
				blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Please choose a operation." );
				if( selected != old_selected ) blitFillBox( 5, blitOffsetLine( 25 ), 480, blitMeasureLine( 5 ), MENU_BGCOLOR );
				switch( selected ){
					case MACRO_RUN_ONCE     : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "Running currently macro for once." ); break;
					case MACRO_RUN_INFINITY : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "Running currently macro for endless." ); break;
					case MACRO_RUN_HALT     : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "Stopping currently running macro." ); break;
					case MACRO_RECORD_START : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "Starting to record a macro." ); break;
					case MACRO_RECORD_STOP  : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "Stopping to record a macro." ); break;
					case MACRO_ANALOG_OPTION: blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "Recording the analog stick movement." ); break;
					case MACRO_EDIT         : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "Editing currently macro." ); break;
					case MACRO_CLEAR        : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "Clearing currently macro." ); break;
					case MACRO_CREATE       : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "Creating new macro." ); break;
					case MACRO_LOAD         : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "Loading a macro from MemoryStick.\n\nIMPORTANT CAUTION:\n  Verify that MemoryStick access indicator is not blinking.\n  Otherwise, will CRASH the currently running game!" ); break;
					case MACRO_SAVE         : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "Saving currently macro to MemoryStick.\n\nIMPORTANT CAUTION:\n  Verify that MemoryStick access indicator is not blinking.\n  Otherwise, will CRASH the currently running game!" ); break;
				}
				blitString( blitOffsetChar( 3 ), blitOffsetLine( 31 ), MENU_FGCOLOR, MENU_BGCOLOR, "\x80\x82 = Move, \x83\x81 = Change toggle, \x85 = Enter, \x86 = Back, START = Exit" );
				old_selected = selected;
				break;
			case MR_ENTER:
				switch( selected ){
					case MACRO_RUN_ONCE    : function = macroRunOnce;     break;
					case MACRO_RUN_INFINITY: function = macroRunInfinity; break;
					case MACRO_RUN_HALT    : function = macroRunInterrupt;break;
					case MACRO_RECORD_START: function = macroRecordStart; break;
					case MACRO_RECORD_STOP : function = macroRecordStop;  break;
					case MACRO_EDIT        : function = macroEdit;        break;
					case MACRO_CLEAR       : function = macroClear;       break;
					case MACRO_CREATE      : function = macroCreate;      break;
					case MACRO_LOAD        : function = macroLoad;        break;
					case MACRO_SAVE        : function = macroSave;        break;
				}
				/* 最後のボタン情報をリセット */
				st_temp_buttons      = 0;
				st_temp_analog_move  = false;
				st_temp_analog_coord = 0;
				st_temp_tick         = 0;
				
				mfClearColor( MENU_BGCOLOR );
				break;
			case MR_BACK:
				return MR_BACK;
		}
	} else{
		if( ( function )( pad_latch, pad_data ) == MR_BACK ){
			function = NULL;
			mfClearScreenWhenVsync();
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
	static bool cur_analog_move = false;
	u64 cur_analog_coord = 0;
	static MacroData *cur_macro;
	
	if( mode == MF_CALL_LATCH || ! pad_data ) return;
	
	cur_buttons = pad_data->Buttons;
	
	/* チェックしないボタンをマスク */
	cur_buttons &= 0x0000FFFF;
	
	if( st_availAnalog == MACRO_ANALOG_ON ){
		if( MACRO_ANALOG_THRESHOLD( 128, 128, 50, pad_data->Lx, pad_data->Ly ) ){
			cur_analog_move  = true;
			cur_analog_coord = MACRO_SET_ANALOG_XY( pad_data->Lx, pad_data->Ly );
		} else{
			cur_analog_move = false;
			cur_analog_coord = MACRO_SET_ANALOG_XY( 128, 128 );
		}
	}
	
	if( ! macromgrGetCount() ) cur_macro = NULL;
	
	if( st_temp_buttons == cur_buttons && ( cur_analog_move == st_temp_analog_move && cur_analog_coord == st_temp_analog_coord ) ){
		u64 tick;
		sceRtcGetCurrentTick( &tick );
		
		if( cur_macro && cur_macro->action == MA_DELAY ){
			u64 diff = ( tick - st_temp_tick ) / 1000;
			cur_macro->data = diff > MACRO_MAX_DELAY ? MACRO_MAX_DELAY : diff;
			return;
		} else{
			cur_macro = macro_append( cur_macro );
			if( ! cur_macro ) return;
			
			cur_macro->action = MA_DELAY;
			cur_macro->data   = 0;
			st_temp_tick      = tick;
		}
	} else{
		unsigned int press_buttons   = ( st_temp_buttons ^ cur_buttons ) & cur_buttons;
		unsigned int release_buttons = ( st_temp_buttons ^ cur_buttons ) & st_temp_buttons;
		
		if( press_buttons || release_buttons ){
			if( ! ( cur_macro->action == MA_DELAY && cur_macro->data == 0 ) ){
				cur_macro = macro_append( cur_macro );
				if( ! cur_macro ) return;
			}
			
			if( press_buttons && release_buttons ){
				cur_macro->action = MA_BUTTONS_CHANGE;
				cur_macro->data   = cur_buttons;
			} else if( press_buttons ){
				cur_macro->action = MA_BUTTONS_PRESS;
				cur_macro->data   = press_buttons;
			} else if( release_buttons ){
				cur_macro->action = MA_BUTTONS_RELEASE;
				cur_macro->data   = release_buttons;
			}
		}
		
		if( ( st_temp_analog_move || cur_analog_move ) && cur_analog_coord != st_temp_analog_coord ){
			cur_macro = macro_append( cur_macro );
			if( ! cur_macro ) return;
			
			if( st_temp_analog_move && ! cur_analog_move ){
				cur_macro->action = MA_ANALOG_NEUTRAL;
				cur_macro->data   = 0;
			} else{
				cur_macro->action = MA_ANALOG_MOVE;
				cur_macro->data   = cur_analog_coord;
			}
		}
	}
	
	st_temp_buttons      = cur_buttons;
	st_temp_analog_move  = cur_analog_move;
	st_temp_analog_coord = cur_analog_coord;
}

static void macro_trace( MfCallMode mode, SceCtrlData *pad_data, void *argp )
{
	static MacroData *cur_macro = NULL;
	bool forward = true;
	
	if( ! macromgrGetCount() || ! pad_data || st_runMode != MRM_TRACE ){
		cur_macro = NULL;
		return;
	} else if( mode == MF_CALL_LATCH ){
		pad_data->Buttons |= st_temp_buttons;
		return;
	}
	
	if( ! cur_macro ) cur_macro = macromgrGetFirst();
	
	switch( cur_macro->action ){
		case MA_DELAY:
			{
				u64 tick;
				sceRtcGetCurrentTick( &tick );
				
				if( ! cur_macro->data ){
					break;
				} else if( ! st_temp_tick ){
					st_temp_tick = tick;
					forward = false;
				} else if( ( tick - st_temp_tick ) > cur_macro->data * 1000 ){
					st_temp_tick = 0;
				} else{
					forward = false;
				}
			}
			break;
		case MA_BUTTONS_PRESS:
			st_temp_buttons |= cur_macro->data;
			break;
		case MA_BUTTONS_RELEASE:
			st_temp_buttons ^= cur_macro->data;
			break;
		case MA_BUTTONS_CHANGE:
			st_temp_buttons = cur_macro->data;
			break;
		case MA_ANALOG_NEUTRAL:
			st_temp_analog_move  = false;
			st_temp_analog_coord = MACRO_SET_ANALOG_XY( 128, 128 );
			break;
		case MA_ANALOG_MOVE:
			st_temp_analog_move  = true;
			st_temp_analog_coord = cur_macro->data;
			break;
	}
	
	pad_data->Buttons |= st_temp_buttons;
	if( st_temp_analog_move ){
		pad_data->Lx = MACRO_GET_ANALOG_X( st_temp_analog_coord );
		pad_data->Ly = MACRO_GET_ANALOG_Y( st_temp_analog_coord );
	}
	
	if( ! forward ) return;
	
	if( cur_macro->next ){
		cur_macro = cur_macro->next;
	} else if( ! st_runLoop || --st_runLoop ){
		cur_macro = macromgrGetFirst();
	} else{
		cur_macro = NULL;
		st_runMode = MRM_NONE;
	}
}

static bool macro_is_busy( void )
{
	if( st_runMode != MRM_NONE ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Macro function is busy." );
		mfWaitScreenReload( 1 );
		return true;
	}
	return false;

}

static bool macro_is_avail( void )
{
	if( ! macromgrGetCount() ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "No current macro." );
		mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
		return false;
	}
	return true;
}

static bool macro_is_enabled_mf_engine( void )
{
	if( mfIsDisabled() ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "MacroFire Engine is currently off." );
		mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
		return false;
	}
	return true;
}

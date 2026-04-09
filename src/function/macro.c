/*
	Macro
*/

#include "macro.h"

static int st_runLoop          = 0;
static MacroRunMode st_runMode = MRM_NONE;
static MacroData *st_curMacro  = NULL;
static unsigned int st_temp_buttons;
static u64 st_temp_tick;

#define MACRO_LOAD         0
#define MACRO_SAVE         1
#define MACRO_EDIT         3
#define MACRO_CLEAR        4
#define MACRO_RECORD_START 6
#define MACRO_RECORD_STOP  7
#define MACRO_RUN_ONCE     9
#define MACRO_RUN_INFINITY 10
#define MACRO_RUN_HALT     11
static MfMenuItem st_macroMenu[] = {
	{ MT_ANCHOR, 0, "Load from MS (not impremented yet)", { 0 } },
	{ MT_ANCHOR, 0, "Save to MS (not impremented yet)",   { 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_ANCHOR, 0, "Edit macro", { 0 } },
	{ MT_ANCHOR, 0, "Clear macro",  { 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_ANCHOR, 0, "Start recording", { 0 } },
	{ MT_ANCHOR, 0, "Stop recording",  { 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_ANCHOR, 0, "Run once",     { 0 } },
	{ MT_ANCHOR, 0, "Run infinity", { 0 } },
	{ MT_ANCHOR, 0, "Stop macro",   { 0 } }
};

static MacroData *macro_add_data( MacroData **macro );
static void macro_record( HookCaller caller, SceCtrlData *pad_data, void *argp );
static void macro_trace( HookCaller caller, SceCtrlData *pad_data, void *argp );
static bool macro_is_busy( void );
static bool macro_is_avail( void );
static bool macro_is_enabled_mf_engine( void );

void macroInit( void )
{
	return;
}

void macroTerm( void )
{
	macroClear( NULL, NULL );
}

MfMenuReturnCode macroRunInterrupt( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( st_runMode != MRM_TRACE ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Not ran yet.", MENU_FGCOLOR, MENU_BGCOLOR );
	} else{
		st_runMode = MRM_NONE;
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Macro halted.", MENU_FGCOLOR, MENU_BGCOLOR );
		macro_trace( CALL_PEEK_BUFFER_POSITIVE, NULL, NULL );
	}
	mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
	return MR_BACK;
}

MfMenuReturnCode macroRunOnce( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( macro_is_busy() || ! macro_is_avail() || ! macro_is_enabled_mf_engine() ) return MR_BACK;
	
	st_runLoop = 1;
	st_runMode = MRM_TRACE;
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Run Once...", MENU_FGCOLOR, MENU_BGCOLOR );
	mfWaitScreenReload( 1 );
	mfMenuQuit();
	
	return MR_BACK;
}

MfMenuReturnCode macroRunInfinity( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( macro_is_busy() || ! macro_is_avail() || ! macro_is_enabled_mf_engine() ) return MR_BACK;
	
	st_runLoop = 0;
	st_runMode = MRM_TRACE;
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Run Infinity...", MENU_FGCOLOR, MENU_BGCOLOR );
	mfWaitScreenReload( 1 );
	mfMenuQuit();
	
	return MR_BACK;

}

void macroMain( HookCaller caller, SceCtrlData *pad_data, void *argp )
{
	switch( st_runMode ){
		case MRM_NONE: break;
		case MRM_TRACE:  macro_trace( caller, pad_data, argp );  break;
		case MRM_RECORD: macro_record( caller, pad_data, argp ); break;
	}
}

MfMenuReturnCode macroLoad( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( macro_is_busy() ) return MR_BACK;
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Not impremented yet.", MENU_FGCOLOR, MENU_BGCOLOR );
	mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
	return MR_BACK;
}

MfMenuReturnCode macroSave( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( macro_is_busy() || ! macro_is_avail() ) return MR_BACK;
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Not impremented yet.", MENU_FGCOLOR, MENU_BGCOLOR );
	mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
	return MR_BACK;
}

MfMenuReturnCode macroEdit( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( macro_is_busy() || ! macro_is_avail() ) return MR_BACK;
	
	return macroeditorMain( pad_latch, pad_data, st_curMacro );
}

MfMenuReturnCode macroClear( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	MacroData *macro_data;
	
	if( macro_is_busy() || ! macro_is_avail() ) return MR_BACK;
	
	if( ! st_curMacro->next ){
		MemSceFree( st_curMacro );
	} else{
		for( macro_data = st_curMacro->next; macro_data->next; macro_data = macro_data->next ){
			MemSceFree( macro_data->prev );
		}
		MemSceFree( macro_data->prev );
		MemSceFree( macro_data );
	}
	
	st_curMacro = NULL;
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Clearing current macro...", MENU_FGCOLOR, MENU_BGCOLOR );
	mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
	
	return MR_BACK;
}

MfMenuReturnCode macroRecordStop( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	if( st_runMode == MRM_RECORD ){
		st_runMode = MRM_NONE;
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Stopping macro recording...", MENU_FGCOLOR, MENU_BGCOLOR );
	} else{
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Not recorded yet.", MENU_FGCOLOR, MENU_BGCOLOR );
	}
	
	mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
	return MR_BACK;
}

MfMenuReturnCode macroRecordStart( SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	static int sec = 3;
	
	if( macro_is_busy() || ! macro_is_enabled_mf_engine() ) return MR_BACK;
	
	if( st_runMode == MRM_RECORD ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Already started recording", MENU_FGCOLOR, MENU_BGCOLOR );
		mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
		return MR_BACK;
	}
	
	/* 既にマクロがあればクリア */
	if( st_curMacro ) macroClear( NULL, NULL );
	
	while( sec-- ){
		char cntdown[16];
		
		blitFillBox( 3, 50, 480, 8, MENU_BGCOLOR );
		
		sprintf( cntdown, "%d seconds...", sec + 1 );
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Please RELEASING ALL BUTTONS until returning to the game...", MENU_FGCOLOR, MENU_BGCOLOR );
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 5 ), cntdown, MENU_FGCOLOR, MENU_BGCOLOR );
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
		switch( mfMenuVertical( blitOffsetChar( 5 ), blitOffsetLine( 4 ), BLIT_SCR_WIDTH, st_macroMenu, ARRAY_NUM( st_macroMenu ), &selected ) ){
			case MR_CONTINUE:
				blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Please choose a operation.", MENU_FGCOLOR, MENU_BGCOLOR );
				if( selected != old_selected ) blitFillBox( 5, blitOffsetLine( 25 ), 480, BLIT_CHAR_HEIGHT, MENU_BGCOLOR );
				switch( selected ){
					case MACRO_LOAD        : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), "Loading a macro from MemoryStick.", MENU_FGCOLOR, MENU_BGCOLOR ); break;
					case MACRO_SAVE        : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), "Saving a macro to MemoryStick.", MENU_FGCOLOR, MENU_BGCOLOR ); break;
					case MACRO_EDIT        : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), "Edit current macro.", MENU_FGCOLOR, MENU_BGCOLOR ); break;
					case MACRO_CLEAR       : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), "Clear current macro.", MENU_FGCOLOR, MENU_BGCOLOR ); break;
					case MACRO_RECORD_START: blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), "Starting recording macro.", MENU_FGCOLOR, MENU_BGCOLOR ); break;
					case MACRO_RECORD_STOP : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), "Stopping recording macro.", MENU_FGCOLOR, MENU_BGCOLOR ); break;
					case MACRO_RUN_ONCE    : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), "Run current macro for once.", MENU_FGCOLOR, MENU_BGCOLOR ); break;
					case MACRO_RUN_INFINITY: blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), "Run current macro for endless.", MENU_FGCOLOR, MENU_BGCOLOR ); break;
					case MACRO_RUN_HALT    : blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), "Stop current running macro.", MENU_FGCOLOR, MENU_BGCOLOR ); break;
				}
				blitString( blitOffsetChar( 3 ), blitOffsetLine( 31 ), "\x80 = MoveUp, \x82 = MoveDown, \x85 = Enter, \x86 = Back, START = Exit", MENU_FGCOLOR, MENU_BGCOLOR );
				old_selected = selected;
				break;
			case MR_ENTER:
				switch( selected ){
					case MACRO_LOAD        : function = macroLoad;        break;
					case MACRO_SAVE        : function = macroSave;        break;
					case MACRO_EDIT        : function = macroEdit;        break;
					case MACRO_CLEAR       : function = macroClear;       break;
					case MACRO_RECORD_START: function = macroRecordStart; break;
					case MACRO_RECORD_STOP : function = macroRecordStop;  break;
					case MACRO_RUN_ONCE    : function = macroRunOnce;     break;
					case MACRO_RUN_INFINITY: function = macroRunInfinity; break;
					case MACRO_RUN_HALT    : function = macroRunInterrupt;break;
				}
				/* 最後のボタン情報をリセット */
				st_temp_buttons = 0;
				st_temp_tick    = 0;
				
				mfClearColor( MENU_BGCOLOR );
				break;
			case MR_BACK:
				selected = 0;
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
static MacroData *macro_add_data( MacroData **macro )
{
	if( ! macro ) return NULL;
	
	if( ! st_curMacro ){
		*macro = (MacroData *)MemSceMallocEx( 16, PSP_MEMPART_KERNEL_1, "macroData", PSP_SMEM_Low, sizeof( MacroData ), 0 );
		
		if( *macro ){
			memset( *macro, 0, sizeof( MacroData ) );
			st_curMacro = *macro;
		}
	} else{
		(*macro)->next = (MacroData *)MemSceMallocEx( 16, PSP_MEMPART_KERNEL_1, "macroData", PSP_SMEM_Low, sizeof( MacroData ), 0 );
		if( (*macro)->next ){
			memset( (*macro)->next, 0, sizeof( MacroData ) );
			(*macro)->next->prev = *macro;
		}
		*macro = (*macro)->next;
	}
	return *macro;
}

static void macro_record( HookCaller caller, SceCtrlData *pad_data, void *argp )
{
	unsigned int cur_buttons;
	static MacroData *cur_macro = NULL;
	
	if( ! pad_data ) return;
	
	switch( caller ){
		case CALL_PEEK_LATCH:
		case CALL_READ_LATCH:
			return;
		case CALL_PEEK_BUFFER_NEGATIVE:
		case CALL_READ_BUFFER_NEGATIVE:
			cur_buttons = ~(pad_data->Buttons);
			break;
		case CALL_PEEK_BUFFER_POSITIVE:
		case CALL_READ_BUFFER_POSITIVE:
		default:
			cur_buttons = pad_data->Buttons;
			break;
	}
	
	/* チェックしないボタンをマスク */
	cur_buttons &= 0x0000FFFF;
	
	if( st_temp_buttons == cur_buttons ){
		u64 tick;
		sceRtcGetCurrentTick( &tick );
		
		if( cur_macro && cur_macro->action == MA_DELAY ){
			cur_macro->data = tick - st_temp_tick;
			if( cur_macro->data > MACRO_MAX_DELAY ) cur_macro->data = MACRO_MAX_DELAY;
			return;
		} else{
			macro_add_data( &cur_macro );
			if( ! cur_macro ) return;
			
			cur_macro->action = MA_DELAY;
			cur_macro->data   = 0;
			st_temp_tick      = tick;
		}
	} else{
		unsigned int press_buttons   = ( st_temp_buttons ^ cur_buttons ) & cur_buttons;
		unsigned int release_buttons = ( st_temp_buttons ^ cur_buttons ) & st_temp_buttons;
		
		if( cur_macro && cur_macro->action == MA_DELAY && cur_macro->data != 0 ){
			macro_add_data( &cur_macro );
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
	
	st_temp_buttons = cur_buttons;
}

static void macro_trace( HookCaller caller, SceCtrlData *pad_data, void *argp )
{
	static MacroData *cur_macro = NULL;
	bool forward = true;
	
	if( ! st_curMacro || ! pad_data || st_runMode != MRM_TRACE ){
		cur_macro = NULL;
		return;
	}
	
	if( ! cur_macro ) cur_macro = st_curMacro;
	
	if( caller == CALL_PEEK_LATCH || caller == CALL_READ_LATCH ){
		pad_data->Buttons |= st_temp_buttons;
		return;
	}
	
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
				} else if( ( tick - st_temp_tick ) > cur_macro->data ){
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
	}
	
	switch( caller ){
		case CALL_PEEK_BUFFER_NEGATIVE:
		case CALL_READ_BUFFER_NEGATIVE:
			pad_data->Buttons = ~( ~(pad_data->Buttons) | st_temp_buttons );
			break;
		case CALL_PEEK_BUFFER_POSITIVE:
		case CALL_READ_BUFFER_POSITIVE:
			pad_data->Buttons |= st_temp_buttons;
			break;
		case CALL_PEEK_LATCH:
		case CALL_READ_LATCH:
			break; /* 警告抑止 */
	}
	
	if( ! forward ) return;
	
	if( cur_macro->next ){
		cur_macro = cur_macro->next;
	} else if( ! st_runLoop || --st_runLoop ){
		cur_macro = st_curMacro;
	} else{
		cur_macro = NULL;
		st_runMode = MRM_NONE;
	}
}

static bool macro_is_busy( void )
{
	if( st_runMode != MRM_NONE ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "Macro function is busy.", MENU_FGCOLOR, MENU_BGCOLOR );
		mfWaitScreenReload( 1 );
		return true;
	}
	return false;

}

static bool macro_is_avail( void )
{
	if( ! st_curMacro ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "No current macro.", MENU_FGCOLOR, MENU_BGCOLOR );
		mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
		return false;
	}
	return true;
}

static bool macro_is_enabled_mf_engine( void )
{
	if( mfIsDisabled() ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), "MacroFire Engine is currently off.", MENU_FGCOLOR, MENU_BGCOLOR );
		mfWaitScreenReload( MACRO_NOTICE_DISPLAY_SEC );
		return false;
	}
	return true;
}

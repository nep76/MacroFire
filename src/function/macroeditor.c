/*
	macroeditor.c
*/

#include "macroeditor.h"

static int st_selected_macro_num = 0;
static int st_macro_all_lines = 0;

#define MACROEDITOR_EDIT_DATA  0
#define MACROEDITOR_CH_TYPE    1
#define MACROEDITOR_INS_BEFORE 3
#define MACROEDITOR_INS_AFTER  4
#define MACROEDITOR_DELETE     5
static MfMenuItem st_editmenu[] = {
	{ MT_ANCHOR, 0, "Edit data", { 0 } },
	{ MT_ANCHOR, 0, "Change type", { 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_ANCHOR, 0, "Insert before", { 0 } },
	{ MT_ANCHOR, 0, "Insert after", { 0 } },
	{ MT_ANCHOR, 0, "Delete", { 0 } }
};

#define MACROEDITOR_WAITMS_DIGIT 9
char st_edit_waitms[MACROEDITOR_WAITMS_DIGIT + 1]; /* 999,999,999マイクロ秒(999秒)まで */

#define MACROEDITOR_COORD_DIGIT 3
char st_edit_coord_x[MACROEDITOR_COORD_DIGIT + 1];
char st_edit_coord_y[MACROEDITOR_COORD_DIGIT + 1];

#define MACROEDITOR_TYPE_DELAY           0
#define MACROEDITOR_TYPE_BUTTONS_PRESS   1
#define MACROEDITOR_TYPE_BUTTONS_RELEASE 2
#define MACROEDITOR_TYPE_BUTTONS_CHANGE  3
#define MACROEDITOR_TYPE_ANALOG_NEUTRAL  4
#define MACROEDITOR_TYPE_ANALOG_MOVE     5
static MfMenuItem st_edit_chtype[] = {
	{ MT_ANCHOR, 0, "Delay", { 0 } },
	{ MT_ANCHOR, 0, "Buttons press", { 0 } },
	{ MT_ANCHOR, 0, "Buttons release", { 0 } },
	{ MT_ANCHOR, 0, "Buttons change", { 0 } },
	{ MT_ANCHOR, 0, "Analog neutral", { 0 } },
	{ MT_ANCHOR, 0, "Analog move",    { 0 } }
};

static unsigned int macroeditor_count_macro_lines( MacroData *macro );
static void macroeditor_print_button_symbol( unsigned int buttons, char *command, size_t len );

static MfMenuReturnCode macroeditor_menu( SceCtrlLatch *pad_latch, MacroData *macro, unsigned int num );
static MfMenuReturnCode macroeditor_edit_data( SceCtrlLatch *pad_latch, MacroData *macro );
static MfMenuReturnCode macroeditor_ch_type( SceCtrlLatch *pad_latch, MacroData *macro );
static MfMenuReturnCode macroeditor_ins_before( SceCtrlLatch *pad_latch, MacroData *macro );
static MfMenuReturnCode macroeditor_ins_after( SceCtrlLatch *pad_latch, MacroData *macro );
static MfMenuReturnCode macroeditor_delete( SceCtrlLatch *pad_latch, MacroData *macro );


void macroeditorReset( void )
{
	st_macro_all_lines    = 0;
	st_selected_macro_num = 0;
}

MfMenuReturnCode macroeditorMain( SceCtrlLatch *pad_latch, SceCtrlData *pad_data, MacroData *macro )
{
	static bool run_menu = false;
	static int prev_start_line = -1;
	
	int y = 2, line = 1, rest;
	bool redraw = false;
	
	/* メニュー表示 */
	if( run_menu ){
		switch( macroeditor_menu( pad_latch, macro, st_selected_macro_num ) ){
			case MR_CONTINUE:
				run_menu = true;
				break;
			case MR_BACK:
				run_menu = false;
				mfMenuEnableInterrupt();
				break;
			case MR_ENTER:    break; /* MR_ENTERが返ることはない。コンパイラの警告抑止の為。 */
		}
		return MR_CONTINUE;
	}
	
	/* 操作 */
	if( ! st_macro_all_lines ){
		st_macro_all_lines = macroeditor_count_macro_lines( macro );
		if( st_selected_macro_num > st_macro_all_lines ) st_selected_macro_num = st_macro_all_lines;
	}
	
	if( pad_latch->uiMake & PSP_CTRL_CROSS ){
		macroeditorReset();
		return MR_BACK;
	} else if( pad_latch->uiMake & PSP_CTRL_CIRCLE ){
		/* 0にリセットして、編集後に再取得させる */
		st_macro_all_lines = 0;
		
		run_menu = true;
		mfMenuDisableInterrupt();
		
		return MR_CONTINUE;
	} else if( pad_latch->uiMake & PSP_CTRL_UP ){
		st_selected_macro_num--;
	} else if( pad_latch->uiMake & PSP_CTRL_DOWN ){
		st_selected_macro_num++;
	} else if( pad_latch->uiMake & PSP_CTRL_LEFT ){
		st_selected_macro_num -= MACROEDITOR_LINES_PER_PAGE;
	} else if( pad_latch->uiMake & PSP_CTRL_RIGHT ){
		st_selected_macro_num += MACROEDITOR_LINES_PER_PAGE;
	}
	
	/* 負数の場合は先頭で上を押されていたら末尾へ */
	if( st_selected_macro_num < 0 ){
		if( pad_latch->uiMake & PSP_CTRL_UP ){
			st_selected_macro_num = st_macro_all_lines;
		} else{
			st_selected_macro_num = 0;
		}
	}
	/* 最大数を超えている場合は末尾で下を押されているので先頭へ */
	if( st_selected_macro_num > st_macro_all_lines ){
		if( pad_latch->uiMake & PSP_CTRL_DOWN ){
			st_selected_macro_num = 0;
		} else{
			st_selected_macro_num = st_macro_all_lines;
		}
	}
	
	/* スクロール */
	for( rest = st_selected_macro_num - ( MACROEDITOR_LINES_PER_PAGE >> 1 ); rest > 0 && macro; ( macro = macro->next ), line++, rest-- ){
		if( st_macro_all_lines - ( line - 1 ) < MACROEDITOR_LINES_PER_PAGE ){ break; }
		continue;
	}
	
	if( prev_start_line != line - 1 ){
		redraw = true;
		prev_start_line = line - 1;
	}
	
	/* 表示開始 */
	for( rest = MACROEDITOR_LINES_PER_PAGE; macro && rest; ( macro = macro->next ), line++, rest-- ){
		char command[255];
		
		if( macro->action == MA_DELAY ){
			snprintf( command, sizeof( command ), "%03d: Delay -----------> %llu ms", line, (uint64_t)macro->data );
		} else if( macro->action == MA_BUTTONS_PRESS ){
			snprintf( command, sizeof( command ), "%03d: Buttons press ---> ", line );
			macroeditor_print_button_symbol( macro->data, command, 255 );
		} else if( macro->action == MA_BUTTONS_RELEASE ){
			snprintf( command, sizeof( command ), "%03d: Buttons release -> ", line );
			macroeditor_print_button_symbol( macro->data, command, 255 );
		} else if( macro->action == MA_BUTTONS_CHANGE ){
			snprintf( command, sizeof( command ), "%03d: Buttons change --> ", line );
			macroeditor_print_button_symbol( macro->data, command, 255 );
		} else if( macro->action == MA_ANALOG_NEUTRAL ){
			snprintf( command, sizeof( command ), "%03d: Analog(x,y)------> NEUTRAL", line );
		} else if( macro->action == MA_ANALOG_MOVE ){
			snprintf( command, sizeof( command ), "%03d: Analog(x,y)------> %lu, %lu", line, (uint32_t)MACRO_GET_ANALOG_X( macro->data ), (uint32_t)MACRO_GET_ANALOG_Y( macro->data ) );
		}
		
		if( redraw ) blitFillBox( 0, blitOffsetLine( y ), 480, blitMeasureLine( 2 ), MENU_BGCOLOR );
		
		if( line - 1 == st_selected_macro_num ){
			blitString( blitOffsetChar( 3 ), blitOffsetLine( y ), MENU_FCCOLOR, MENU_BGCOLOR, command );
		} else{
			blitString( blitOffsetChar( 3 ), blitOffsetLine( y ), MENU_FGCOLOR, MENU_BGCOLOR, command );
		}
		y++;
	}
	
	if( rest && redraw )
		blitFillBox( 0, blitOffsetLine( y ), 480, blitMeasureLine( rest ), MENU_BGCOLOR );
	
	redraw = false;
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 31 ), MENU_FGCOLOR, MENU_BGCOLOR, "\x80 = MoveUp, \x82 = MoveDown, \x83 = PageUp, \x81 = PageDown\n\x85 = Edit menu, \x86 = Back, START = Exit" );
	
	return MR_CONTINUE;
}

/*-------------------------------------
	Static関数
--------------------------------------*/
static unsigned int macroeditor_count_macro_lines( MacroData *macro )
{
	unsigned int i;
	for( i = 0; macro->next; ( macro = macro->next ), i++ );
	
	return i;
}

static void macroeditor_print_button_symbol( unsigned int buttons, char *command, size_t len )
{
	if( buttons & PSP_CTRL_UP       ) safe_strncat( command, "\x80 ", len );
	if( buttons & PSP_CTRL_RIGHT    ) safe_strncat( command, "\x81 ", len );
	if( buttons & PSP_CTRL_DOWN     ) safe_strncat( command, "\x82 ", len );
	if( buttons & PSP_CTRL_LEFT     ) safe_strncat( command, "\x83 ", len );
	if( buttons & PSP_CTRL_LTRIGGER ) safe_strncat( command, "L ", len );
	if( buttons & PSP_CTRL_RTRIGGER ) safe_strncat( command, "R ", len );
	if( buttons & PSP_CTRL_TRIANGLE ) safe_strncat( command, "\x84 ", len );
	if( buttons & PSP_CTRL_CIRCLE   ) safe_strncat( command, "\x85 ", len );
	if( buttons & PSP_CTRL_CROSS    ) safe_strncat( command, "\x86 ", len );
	if( buttons & PSP_CTRL_SQUARE   ) safe_strncat( command, "\x87 ", len );
	if( buttons & PSP_CTRL_SELECT   ) safe_strncat( command, "SELECT ", len );
	if( buttons & PSP_CTRL_START    ) safe_strncat( command, "START ", len );
}

static MfMenuReturnCode macroeditor_menu( SceCtrlLatch *pad_latch, MacroData *macro, unsigned int num )
{
	MfMenuReturnCode rc = MR_CONTINUE;
	static int selected = -1;
	static MacroeditorEditFunction function = NULL;
	static MacroData *req_macro;
	
	/* selectedが0未満の場合は初回の呼び出しなので描画範囲を塗りつぶし */
	if( selected < 0 ){
		blitFillBox( MACROEDITOR_MAINMENU_POS_X, MACROEDITOR_MAINMENU_POS_Y, blitMeasureChar( 15 ), blitMeasureLine( 8 ), MENU_BGCOLOR );
		blitLineBox( MACROEDITOR_MAINMENU_POS_X, MACROEDITOR_MAINMENU_POS_Y, blitMeasureChar( 15 ), blitMeasureLine( 8 ), MENU_FGCOLOR );
		selected = 0;
	}
	
	if( ! function ){
		switch( mfMenuVertical( MACROEDITOR_MAINMENU_POS_X + BLIT_CHAR_WIDTH, MACROEDITOR_MAINMENU_POS_Y + BLIT_CHAR_HEIGHT, blitMeasureChar( 13 ), st_editmenu, ARRAY_NUM( st_editmenu ), &selected ) ){
			case MR_CONTINUE: break;
			case MR_ENTER:
				switch( selected ){
					case MACROEDITOR_EDIT_DATA:  function = macroeditor_edit_data;  break;
					case MACROEDITOR_CH_TYPE:    function = macroeditor_ch_type;    break;
					case MACROEDITOR_INS_BEFORE: function = macroeditor_ins_before; break;
					case MACROEDITOR_INS_AFTER:  function = macroeditor_ins_after;  break;
					case MACROEDITOR_DELETE:     function = macroeditor_delete;     break;
				}
				for( req_macro = macro; num; num-- ){
					if( req_macro->next ){
						req_macro = req_macro->next;
					} else{
						break;
					}
				}
				break;
			case MR_BACK:
				rc = MR_BACK;
		}
	} else{
		if( ( function )( pad_latch, req_macro ) == MR_BACK ){
				function = NULL;
				rc = MR_BACK;
		}
	}
	
	if( rc == MR_BACK ){
		mfClearColor ( MENU_BGCOLOR );
		selected = -1;
	}
	
	return rc;
}

static MfMenuReturnCode macroeditor_edit_delay( SceCtrlLatch *pad_latch, MacroData *macro, int *selected )
{
	CmndlgGetDigitsData cgd[1];
	cgd[0].title = "Set delay (1sec = 1000ms)";
	cgd[0].unit  = "ms";
	cgd[0].number = (long *)&(macro->data);
	cgd[0].numDigits = 9;
	
	cmndlgGetDigits( MACROEDITOR_EDIT_WAITMS_POS_X, MACROEDITOR_EDIT_WAITMS_POS_Y, cgd, 1 );
	
	return MR_BACK;
}

static MfMenuReturnCode macroeditor_edit_buttons( SceCtrlLatch *pad_latch, MacroData *macro, int *selected )
{
	CmndlgGetButtonsData cgb[1];
	
	cgb[0].title   = "Set buttons state";
	cgb[0].buttons = (unsigned int *)&(macro->data);
	cgb[0].btnMask = PSP_CTRL_NOTE | PSP_CTRL_SCREEN | PSP_CTRL_VOLUP | PSP_CTRL_VOLDOWN;
	
	cmndlgGetButtons( MACROEDITOR_EDIT_BUTTONS_POS_X, MACROEDITOR_EDIT_BUTTONS_POS_Y, cgb, 1 );
	
	return MR_BACK;
}

static MfMenuReturnCode macroeditor_edit_analog( SceCtrlLatch *pad_latch, MacroData *macro, int *selected )
{
	CmndlgGetDigitsData cgd[2];
	int x, y;
	
	x = MACRO_GET_ANALOG_X( macro->data );
	y = MACRO_GET_ANALOG_Y( macro->data );
	
	cgd[0].title     = "Set analog X-coordinate";
	cgd[0].unit      = "(0-255)";
	cgd[0].number    = (long *)&x;
	cgd[0].numDigits = 3;
	cgd[1].title     = "Set analog Y-coordinate";
	cgd[1].unit      = "(0-255)";
	cgd[1].number    = (long *)&y;
	cgd[1].numDigits = 3;
	
	cmndlgGetDigits( MACROEDITOR_EDIT_WAITMS_POS_X, MACROEDITOR_EDIT_WAITMS_POS_Y, cgd, 2 );
	
	if( x > 255 ) x = 255;
	if( y > 255 ) y = 255;
	macro->data = MACRO_SET_ANALOG_XY( x, y );
	
	return MR_BACK;
}

static MfMenuReturnCode macroeditor_edit_data( SceCtrlLatch *pad_latch, MacroData *macro )
{
	static int selected = -1;
	
	if( macro->action == MA_DELAY ){
		return macroeditor_edit_delay( pad_latch, macro, &selected );
	} else if( macro->action == MA_BUTTONS_PRESS || macro->action == MA_BUTTONS_RELEASE || macro->action == MA_BUTTONS_CHANGE ){
		return macroeditor_edit_buttons( pad_latch, macro, &selected );
	} else if( macro->action == MA_ANALOG_MOVE ){
		return macroeditor_edit_analog( pad_latch, macro, &selected );
	}
	
	return MR_BACK;
}

static MfMenuReturnCode macroeditor_ch_type( SceCtrlLatch *pad_latch, MacroData *macro )
{
	static int selected = -1;
	
	if( selected < 0 ){
		blitFillBox( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, blitMeasureChar( 17 ), blitMeasureLine( 8 ), MENU_BGCOLOR );
		blitLineBox( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, blitMeasureChar( 17 ), blitMeasureLine( 8 ), MENU_FGCOLOR );
		selected = 0;
	}
	
	switch( mfMenuVertical( MACROEDITOR_EDIT_TYPE_POS_X + BLIT_CHAR_WIDTH, MACROEDITOR_EDIT_TYPE_POS_Y + BLIT_CHAR_HEIGHT, blitMeasureChar( 15 ), st_edit_chtype, ARRAY_NUM( st_edit_chtype ), &selected ) ){
		case MR_CONTINUE: return MR_CONTINUE;
		case MR_ENTER:
			switch( selected ){
				case MACROEDITOR_TYPE_DELAY:
					macro->action = MA_DELAY;
					macro->data   = 0;
					break;
				case MACROEDITOR_TYPE_BUTTONS_PRESS:
					macro->action = MA_BUTTONS_PRESS;
					macro->data   = 0;
					break;
				case MACROEDITOR_TYPE_BUTTONS_RELEASE:
					macro->action = MA_BUTTONS_RELEASE;
					macro->data   = 0;
					break;
				case MACROEDITOR_TYPE_BUTTONS_CHANGE:
					macro->action = MA_BUTTONS_CHANGE;
					macro->data   = 0;
					break;
				case MACROEDITOR_TYPE_ANALOG_NEUTRAL:
					macro->action = MA_ANALOG_NEUTRAL;
					macro->data   = 0;
					break;
				case MACROEDITOR_TYPE_ANALOG_MOVE:
					macro->action = MA_ANALOG_MOVE;
					macro->data   = MACRO_SET_ANALOG_XY( 128, 128 );
					break;
			}
		case MR_BACK:
			selected = -1;
			break;
	}
	
	return MR_BACK;
}

static MfMenuReturnCode macroeditor_ins_before( SceCtrlLatch *pad_latch, MacroData *macro )
{
	macromgrInsert( MIP_BEFORE, macro );
	
	return MR_BACK;
}

static MfMenuReturnCode macroeditor_ins_after( SceCtrlLatch *pad_latch, MacroData *macro )
{
	macromgrInsert( MIP_AFTER, macro );
	
	/* 後ろに追加したのでフォーカスを一つ次へずらす */
	st_selected_macro_num++;
	
	return MR_BACK;
}

static MfMenuReturnCode macroeditor_delete( SceCtrlLatch *pad_latch, MacroData *macro )
{
	macromgrRemove( macro );
	return MR_BACK;
}

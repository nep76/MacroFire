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

#define MACROEDITOR_BUTTON_CIRCLE    0
#define MACROEDITOR_BUTTON_CROSS     1
#define MACROEDITOR_BUTTON_SQUARE    2
#define MACROEDITOR_BUTTON_TRIANGLE  3
#define MACROEDITOR_BUTTON_UP        4
#define MACROEDITOR_BUTTON_RIGHT     5
#define MACROEDITOR_BUTTON_DOWN      6
#define MACROEDITOR_BUTTON_LEFT      7
#define MACROEDITOR_BUTTON_LTRIG     8
#define MACROEDITOR_BUTTON_RTRIG     9
#define MACROEDITOR_BUTTON_SELECT   10
#define MACROEDITOR_BUTTON_START    11
static int st_buttons[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static MfMenuItem st_edit_buttons[] = {
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_CIRCLE],   "Circle  ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_CROSS],    "Cross   ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_SQUARE],   "Square  ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_TRIANGLE], "Triangle", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_UP],       "Up      ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_RIGHT],    "Right   ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_DOWN],     "Down    ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_LEFT],     "Left    ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_LTRIG],    "L-trig  ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_RTRIG],    "R-trig  ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_SELECT],   "SELECT  ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_START],    "START   ", { "OFF", "ON", 0 } },
};

#define MACROEDITOR_WAITMS_DIGIT 10
char st_edit_waitms[MACROEDITOR_WAITMS_DIGIT]; /* 999,999,999マイクロ秒(999秒)まで */

#define MACROEDITOR_TYPE_DELAY           0
#define MACROEDITOR_TYPE_BUTTONS_PRESS   1
#define MACROEDITOR_TYPE_BUTTONS_RELEASE 2
#define MACROEDITOR_TYPE_BUTTONS_CHANGE  3
static MfMenuItem st_edit_chtype[] = {
	{ MT_ANCHOR, 0, "Delay", { 0 } },
	{ MT_ANCHOR, 0, "Buttons press", { 0 } },
	{ MT_ANCHOR, 0, "Buttons release", { 0 } },
	{ MT_ANCHOR, 0, "Buttons change", { 0 } }
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
		/* 0にリセットして、編集後に再取得するよう促す */
		st_macro_all_lines = 0;
		
		run_menu = true;
		mfMenuDisableInterrupt();
		
		return MR_CONTINUE;
	} else if( pad_latch->uiMake & PSP_CTRL_UP ){
		if( st_selected_macro_num > MACROEDITOR_LINES_PER_PAGE >> 1 ) redraw = true;
		st_selected_macro_num--;
	} else if( pad_latch->uiMake & PSP_CTRL_DOWN ){
		st_selected_macro_num++;
		if( st_selected_macro_num > MACROEDITOR_LINES_PER_PAGE >> 1 ) redraw = true;
	} else if( pad_latch->uiMake & PSP_CTRL_LEFT ){
		if( st_selected_macro_num > MACROEDITOR_LINES_PER_PAGE >> 1 ) redraw = true;
		st_selected_macro_num -= MACROEDITOR_LINES_PER_PAGE;
	} else if( pad_latch->uiMake & PSP_CTRL_RIGHT ){
		st_selected_macro_num += MACROEDITOR_LINES_PER_PAGE;
		if( st_selected_macro_num > MACROEDITOR_LINES_PER_PAGE >> 1 ) redraw = true;
	}
	
	if( st_selected_macro_num < 0 ){
		st_selected_macro_num = 0;
	}
	if( st_selected_macro_num > st_macro_all_lines ){
		st_selected_macro_num = st_macro_all_lines;
	}
	
	/* スクロール */
	for( rest = st_selected_macro_num - ( MACROEDITOR_LINES_PER_PAGE >> 1 ); rest > 0 && macro; ( macro = macro->next ), line++, rest-- ) continue;
	
	/* 表示開始 */
	for( rest = 28; macro && rest; ( macro = macro->next ), line++, rest-- ){
		char command[255];
		
		if( macro->action == MA_DELAY ){
			snprintf( command, sizeof( command ), "%03d: Delay -----------> %lu ms", line, (unsigned long int)macro->data );
		} else if( macro->action == MA_BUTTONS_PRESS ){
			snprintf( command, sizeof( command ), "%03d: Buttons press ---> ", line );
			macroeditor_print_button_symbol( macro->data, command, 255 );
		} else if( macro->action == MA_BUTTONS_RELEASE ){
			snprintf( command, sizeof( command ), "%03d: Buttons release -> ", line );
			macroeditor_print_button_symbol( macro->data, command, 255 );
		} else if( macro->action == MA_BUTTONS_CHANGE ){
			snprintf( command, sizeof( command ), "%03d: Buttons change --> ", line );
			macroeditor_print_button_symbol( macro->data, command, 255 );
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
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 31 ), MENU_FGCOLOR, MENU_BGCOLOR, "\x80 = MoveUp, \x82 = MoveDown, \x83 = PageUp, \x81 = PageDown\n\x85 = Edit, \x86 = Back, START = Exit" );
	
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

static MfMenuReturnCode macroeditor_edit_data( SceCtrlLatch *pad_latch, MacroData *macro )
{
	static int selected = -1;
	
	if( macro->action == MA_DELAY ){
		int i;
		
		if( selected < 0 ){
			blitFillBox( MACROEDITOR_EDIT_WAITMS_POS_X, MACROEDITOR_EDIT_WAITMS_POS_Y, blitMeasureChar( 30 ), blitMeasureLine( 9 ), MENU_BGCOLOR );
			blitLineBox( MACROEDITOR_EDIT_WAITMS_POS_X, MACROEDITOR_EDIT_WAITMS_POS_Y, blitMeasureChar( 30 ), blitMeasureLine( 9 ), MENU_FGCOLOR );
			
			if( macro->data > MACRO_MAX_DELAY ) macro->data = MACRO_MAX_DELAY;
			
			/*
				フォーマットに%020lluを使うと落ちる。
				unsigned long long intを上手く扱えない模様。
				やむなくunsigned long intを扱うことに。
			*/
			sprintf( st_edit_waitms, "%09lu", (unsigned long int)macro->data );
			
			selected = MACROEDITOR_WAITMS_DIGIT - 2;
		}
		
		if( pad_latch->uiMake & PSP_CTRL_CROSS ){
			selected = -1;
			return MR_BACK;
		} else if( pad_latch->uiMake & PSP_CTRL_CIRCLE ){
			macro->data = strtoul( st_edit_waitms, NULL, 10 );
			selected = -1;
			return MR_BACK;
		} else if( pad_latch->uiMake & PSP_CTRL_UP ){
			if( st_edit_waitms[selected] == '9' ){
				st_edit_waitms[selected] = '0';
			} else{
				st_edit_waitms[selected]++;
			}
		} else if( pad_latch->uiMake & PSP_CTRL_DOWN ){
			if( st_edit_waitms[selected] == '0' ){
				st_edit_waitms[selected] = '9';
			} else{
				st_edit_waitms[selected]--;
			}
		} else if( pad_latch->uiMake & PSP_CTRL_LEFT && selected ){
			selected--;
		} else if( pad_latch->uiMake & PSP_CTRL_RIGHT && selected < MACROEDITOR_WAITMS_DIGIT - 2 ){
			selected++;
		}
		
		/* 入力行描画 */
		for( i = 0; st_edit_waitms[i] != '\0'; i++ ){
			char editnum[6];
			if( i == selected ){
				editnum[0] = '\x80';            editnum[1] = '\n';
				editnum[2] = st_edit_waitms[i]; editnum[3] = '\n';
				editnum[4] = '\x82';            editnum[5] = '\0';
				blitString(
					MACROEDITOR_OFFSET_X( MACROEDITOR_EDIT_WAITMS_POS_X, i + 1 ),
					MACROEDITOR_OFFSET_Y( MACROEDITOR_EDIT_WAITMS_POS_Y, 1 ),
					MENU_FCCOLOR,
					MENU_BGCOLOR,
					editnum
				);
			} else{
				editnum[0] = '\x20';            editnum[1] = '\n';
				editnum[2] = st_edit_waitms[i]; editnum[3] = '\n';
				editnum[4] = '\x20';            editnum[5] = '\0';
				blitString(
					MACROEDITOR_OFFSET_X( MACROEDITOR_EDIT_WAITMS_POS_X, i + 1 ),
					MACROEDITOR_OFFSET_Y( MACROEDITOR_EDIT_WAITMS_POS_Y, 1 ),
					MENU_FGCOLOR,
					MENU_BGCOLOR,
					editnum
				);
			}
		}
		blitString(
			MACROEDITOR_OFFSET_X( MACROEDITOR_EDIT_WAITMS_POS_X, i + 2 ),
			MACROEDITOR_OFFSET_Y( MACROEDITOR_EDIT_WAITMS_POS_Y, 2 ),
			MENU_FGCOLOR,
			MENU_BGCOLOR,
			"ms"
		);
		
		/* 簡易取説 */
		blitString(
			MACROEDITOR_OFFSET_X( MACROEDITOR_EDIT_WAITMS_POS_X, 1 ),
			MACROEDITOR_OFFSET_Y( MACROEDITOR_EDIT_WAITMS_POS_Y, 4 ),
			MENU_FGCOLOR,
			MENU_BGCOLOR,
			"(1sec = 1000ms)"
		);
		
		blitString(
			MACROEDITOR_OFFSET_X( MACROEDITOR_EDIT_WAITMS_POS_X, 1 ),
			MACROEDITOR_OFFSET_Y( MACROEDITOR_EDIT_WAITMS_POS_Y, 6 ),
			MENU_FGCOLOR,
			MENU_BGCOLOR,
			"\x83\x81 = Move, \x80\x82 = Change value\n\x85 = Enter, \x86 = Cancel"
		);
	} else{
		if( selected < 0 ){
			blitFillBox( MACROEDITOR_EDIT_BUTTONS_POS_X, MACROEDITOR_EDIT_BUTTONS_POS_Y, blitMeasureChar( 15 ), blitMeasureLine( 14 ), MENU_BGCOLOR );
			blitLineBox( MACROEDITOR_EDIT_BUTTONS_POS_X, MACROEDITOR_EDIT_BUTTONS_POS_Y, blitMeasureChar( 15 ), blitMeasureLine( 14 ), MENU_FGCOLOR );
			
			st_buttons[MACROEDITOR_BUTTON_CIRCLE]   = macro->data & PSP_CTRL_CIRCLE   ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_CROSS]    = macro->data & PSP_CTRL_CROSS    ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_SQUARE]   = macro->data & PSP_CTRL_SQUARE   ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_TRIANGLE] = macro->data & PSP_CTRL_TRIANGLE ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_UP]       = macro->data & PSP_CTRL_UP       ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_RIGHT]    = macro->data & PSP_CTRL_RIGHT    ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_DOWN]     = macro->data & PSP_CTRL_DOWN     ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_LEFT]     = macro->data & PSP_CTRL_LEFT     ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_LTRIG]    = macro->data & PSP_CTRL_LTRIGGER ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_RTRIG]    = macro->data & PSP_CTRL_RTRIGGER ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_SELECT]   = macro->data & PSP_CTRL_SELECT   ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_START]    = macro->data & PSP_CTRL_START    ? 1 : 0;
			
			selected = 0;
		}
		
		switch( mfMenuVertical( MACROEDITOR_EDIT_BUTTONS_POS_X + BLIT_CHAR_WIDTH, MACROEDITOR_EDIT_BUTTONS_POS_Y + BLIT_CHAR_HEIGHT, blitMeasureChar( 13 ), st_edit_buttons, ARRAY_NUM( st_edit_buttons ), &selected ) ){
			case MR_ENTER:
			case MR_CONTINUE:
				break;
			case MR_BACK:
				macro->data = 0;
				if( st_buttons[MACROEDITOR_BUTTON_CIRCLE]   ) macro->data |= PSP_CTRL_CIRCLE;
				if( st_buttons[MACROEDITOR_BUTTON_CROSS]    ) macro->data |= PSP_CTRL_CROSS;
				if( st_buttons[MACROEDITOR_BUTTON_SQUARE]   ) macro->data |= PSP_CTRL_SQUARE;
				if( st_buttons[MACROEDITOR_BUTTON_TRIANGLE] ) macro->data |= PSP_CTRL_TRIANGLE;
				if( st_buttons[MACROEDITOR_BUTTON_UP]       ) macro->data |= PSP_CTRL_UP;
				if( st_buttons[MACROEDITOR_BUTTON_RIGHT]    ) macro->data |= PSP_CTRL_RIGHT;
				if( st_buttons[MACROEDITOR_BUTTON_DOWN]     ) macro->data |= PSP_CTRL_DOWN;
				if( st_buttons[MACROEDITOR_BUTTON_LEFT]     ) macro->data |= PSP_CTRL_LEFT;
				if( st_buttons[MACROEDITOR_BUTTON_LTRIG]    ) macro->data |= PSP_CTRL_LTRIGGER;
				if( st_buttons[MACROEDITOR_BUTTON_RTRIG]    ) macro->data |= PSP_CTRL_RTRIGGER;
				if( st_buttons[MACROEDITOR_BUTTON_SELECT]   ) macro->data |= PSP_CTRL_SELECT;
				if( st_buttons[MACROEDITOR_BUTTON_START]    ) macro->data |= PSP_CTRL_START;
				selected = -1;
				return MR_BACK;
		}
	}
	
	return MR_CONTINUE;
}

static MfMenuReturnCode macroeditor_ch_type( SceCtrlLatch *pad_latch, MacroData *macro )
{
	static int selected = -1;
	
	if( selected < 0 ){
		blitFillBox( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, blitMeasureChar( 17 ), blitMeasureLine( 6 ), MENU_BGCOLOR );
		blitLineBox( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, blitMeasureChar( 17 ), blitMeasureLine( 6 ), MENU_FGCOLOR );
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

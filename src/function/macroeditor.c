/*
	macroeditor.c
*/

#include "macroeditor.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static unsigned int macroeditor_count_macro_lines  ( MacroData *macro );
static void         macroeditor_print_button_symbol( unsigned int buttons, char *command, size_t len );
static MfMenuRc     macroeditor_edit_number( char *title, char *unit, int digits, void *number );
static MfMenuRc     macroeditor_edit_buttons( char *title, char *dummy, int avail, void *button );
static MfMenuRc     macroeditor_edit_analog( SceCtrlData *pad_data, MacroData *macro );
static MfMenuRc     macroeditor_edit_rapid( SceCtrlData *pad_data, MacroData *macro );
static MfMenuRc     macroeditor_edit_data( SceCtrlData *pad_data, MacroData *macro );
static MfMenuRc     macroeditor_ch_type( SceCtrlData *pad_data, MacroData *macro );

/*-----------------------------------------------
	ローカル変数と定数
-----------------------------------------------*/
static int st_macro_all_lines    = -1;
static int st_selected_macro_num = 0;

char st_edit_waitms[MACROEDITOR_WAITMS_DIGIT + 1]; /* 999,999,999ミリ秒まで */

char st_edit_coord_x[MACROEDITOR_COORD_DIGIT + 1];
char st_edit_coord_y[MACROEDITOR_COORD_DIGIT + 1];

MfMenuCallback st_callback;
static MfMenuItem st_edit_chtype[] = {
	{ "Delay",           mfMenuDefCallbackProc, &st_callback, { { .pointer = macromgrSet }, { .integer = MA_DELAY }           } },
	{ "Buttons press",   mfMenuDefCallbackProc, &st_callback, { { .pointer = macromgrSet }, { .integer = MA_BUTTONS_PRESS }   } },
	{ "Buttons release", mfMenuDefCallbackProc, &st_callback, { { .pointer = macromgrSet }, { .integer = MA_BUTTONS_RELEASE } } },
	{ "Buttons change",  mfMenuDefCallbackProc, &st_callback, { { .pointer = macromgrSet }, { .integer = MA_BUTTONS_CHANGE }  } },
	{ "Analog move",     mfMenuDefCallbackProc, &st_callback, { { .pointer = macromgrSet }, { .integer = MA_ANALOG_MOVE }     } },
	{ "Rapidfire start", mfMenuDefCallbackProc, &st_callback, { { .pointer = macromgrSet }, { .integer = MA_RAPIDFIRE_START } } },
	{ "Rapidfire stop",  mfMenuDefCallbackProc, &st_callback, { { .pointer = macromgrSet }, { .integer = MA_RAPIDFIRE_STOP }  } },
};

MfMenuCallback st_submenu;
static MfMenuItem st_edit_rapid[] = {
	{ "Rapidfire buttons",       mfMenuDefCallbackProc, &(st_submenu), { { .pointer = macroeditor_edit_buttons }, { .pointer= NULL }, { .string = "Set rapidfire buttons" }, { .pointer = NULL }, { .integer = MACROEDITOR_AVAIL_BUTTONS } } },
	{ "Rapidfire press delay",   mfMenuDefCallbackProc, &(st_submenu), { { .pointer = macroeditor_edit_number },  { .pointer= NULL }, { .string = "Set press delay" },       { .string = "times" }, { .integer = 4 } } },
	{ "Rapidfire release delay", mfMenuDefCallbackProc, &(st_submenu), { { .pointer = macroeditor_edit_number },  { .pointer= NULL }, { .string = "Set release delay" },     { .string = "times" }, { .integer = 4 } } }
};

static MfMenuItem st_edit_analog[] = {
	{ "Analog X-coordinate", mfMenuDefCallbackProc, &(st_submenu), { { .pointer = macroeditor_edit_number }, { .pointer= NULL }, { .string = "Set X-coordinate" }, { .string = "(0-255)" }, { .integer = 3 } } },
	{ "Analog Y-coordinate", mfMenuDefCallbackProc, &(st_submenu), { { .pointer = macroeditor_edit_number }, { .pointer= NULL }, { .string = "Set Y-coordinate" }, { .string = "(0-255)" }, { .integer = 3 } } }
};

/*=============================================*/

void macroeditorReset( void )
{
	st_macro_all_lines    = -1;
	st_selected_macro_num = 0;
}

MfMenuRc macroeditorMain( SceCtrlData *pad_data, MacroData *macro )
{
	static bool                    menu_start = false;
	static MacroeditorEditFunction menu_func  = NULL;
	
	MacroData *selected_macro   = NULL;
	int rest_lines, line_number = 1;
	int y = 4;
	
	if( st_macro_all_lines < 0 ){
		st_macro_all_lines = macroeditor_count_macro_lines( macro );
	}
	
	/* スクロール */
	for( rest_lines = st_selected_macro_num - ( MACROEDITOR_LINES_PER_PAGE >> 1 ); rest_lines > 0 && macro; macro = macro->next, line_number++, rest_lines-- ){
		if( st_macro_all_lines - ( line_number - 1 ) < MACROEDITOR_LINES_PER_PAGE ){
			break;
		}
	}
	
	for( rest_lines = MACROEDITOR_LINES_PER_PAGE; macro && rest_lines; macro = macro->next, rest_lines--, line_number++ ){
		char command[255];
		
		if( macro->action == MA_DELAY ){
			snprintf( command, sizeof( command ), "%03d: Delay -----------> %llu ms", line_number, macro->data );
		} else if( macro->action == MA_BUTTONS_PRESS ){
			snprintf( command, sizeof( command ), "%03d: Buttons press ---> ", line_number );
			macroeditor_print_button_symbol( macro->data, command, sizeof( command ) );
		} else if( macro->action == MA_BUTTONS_RELEASE ){
			snprintf( command, sizeof( command ), "%03d: Buttons release -> ", line_number );
			macroeditor_print_button_symbol( macro->data, command, sizeof( command ) );
		} else if( macro->action == MA_BUTTONS_CHANGE ){
			snprintf( command, sizeof( command ), "%03d: Buttons change --> ", line_number );
			macroeditor_print_button_symbol( macro->data, command, sizeof( command ) );
		} else if( macro->action == MA_ANALOG_MOVE ){
			snprintf( command, sizeof( command ), "%03d: Analog(x,y)------> %d, %d", line_number, (int)MACROMGR_GET_ANALOG_X( macro->data ), (int)MACROMGR_GET_ANALOG_Y( macro->data ) );
		} else if( macro->action == MA_RAPIDFIRE_START ){
			snprintf( command, sizeof( command ), "%03d: Rapidfire start -> PD:%lu RD:%lu ", line_number, MACROMGR_GET_RAPIDPDELAY( macro->sub ), MACROMGR_GET_RAPIDRDELAY( macro->sub ) );
			macroeditor_print_button_symbol( macro->data, command, sizeof( command ) );
		} else if( macro->action == MA_RAPIDFIRE_STOP ){
			snprintf( command, sizeof( command ), "%03d: Rapidfire end ---> ", line_number );
			macroeditor_print_button_symbol( macro->data, command, sizeof( command ) );
		}
		
		if( line_number - 1 == st_selected_macro_num ){
			selected_macro = macro;
			gbPrint( gbOffsetChar( 3 ), gbOffsetLine( y++ ), MFM_TEXT_FCCOLOR, MFM_TEXT_BGCOLOR, command );
		} else{
			gbPrint( gbOffsetChar( 3 ), gbOffsetLine( y++ ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, command );
		}
	}
	
	if( menu_start ){
		switch( menu_func( pad_data, selected_macro ) ){
			case MR_CONTINUE:
				break;
			case MR_ENTER:
			case MR_BACK:
				menu_start = false;
				break;
		}
		return MR_CONTINUE;
	}
	
	gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 31 ) + ( GB_CHAR_HEIGHT >> 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x80\x82 = Move, \x83\x81 = PageMove, \x85 = Edit data, \x84 = Change type, \x87 = Delete\nL = Insert before, R = Insert after, \x86 = Back, START = Exit" );
	
	if( pad_data->Buttons & PSP_CTRL_CROSS ){
		macroeditorReset();
		return MR_BACK;
	} else if( pad_data->Buttons & PSP_CTRL_CIRCLE ){
		menu_start = true;
		menu_func  = macroeditor_edit_data;
	} else if( pad_data->Buttons & PSP_CTRL_TRIANGLE ){
		menu_start = true;
		menu_func  = macroeditor_ch_type;
	} else if( pad_data->Buttons & PSP_CTRL_SQUARE ){
		macromgrRemove( selected_macro );
		if( st_macro_all_lines > 0 ) st_macro_all_lines--;
	} else if( pad_data->Buttons & PSP_CTRL_LTRIGGER ){
		if( macromgrInsertBefore( selected_macro ) ) st_macro_all_lines++;
	} else if( pad_data->Buttons & PSP_CTRL_RTRIGGER ){
		if( macromgrInsertAfter( selected_macro ) ){
			st_macro_all_lines++;
			st_selected_macro_num++;
		}
	} else if( pad_data->Buttons & PSP_CTRL_UP ){
		st_selected_macro_num--;
	} else if( pad_data->Buttons & PSP_CTRL_DOWN ){
		st_selected_macro_num++;
	} else if( pad_data->Buttons & PSP_CTRL_LEFT ){
		st_selected_macro_num -= MACROEDITOR_LINES_PER_PAGE;
	} else if( pad_data->Buttons & PSP_CTRL_RIGHT ){
		st_selected_macro_num += MACROEDITOR_LINES_PER_PAGE;
	}
	
	if( st_selected_macro_num < 0 ){
		if( pad_data->Buttons & PSP_CTRL_UP ){
			st_selected_macro_num = st_macro_all_lines;
		} else{
			st_selected_macro_num = 0;
		}
	}
	
	if( st_selected_macro_num > st_macro_all_lines ){
		if( pad_data->Buttons & PSP_CTRL_DOWN ){
			st_selected_macro_num = 0;
		} else{
			st_selected_macro_num = st_macro_all_lines;
		}
	}
	
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
	if( buttons & PSP_CTRL_CIRCLE   ) strutilSafeCat( command, "\x85 "  , len );
	if( buttons & PSP_CTRL_CROSS    ) strutilSafeCat( command, "\x86 "  , len );
	if( buttons & PSP_CTRL_SQUARE   ) strutilSafeCat( command, "\x87 "  , len );
	if( buttons & PSP_CTRL_TRIANGLE ) strutilSafeCat( command, "\x84 "  , len );
	if( buttons & PSP_CTRL_UP       ) strutilSafeCat( command, "\x80 "  , len );
	if( buttons & PSP_CTRL_RIGHT    ) strutilSafeCat( command, "\x81 "  , len );
	if( buttons & PSP_CTRL_DOWN     ) strutilSafeCat( command, "\x82 "  , len );
	if( buttons & PSP_CTRL_LEFT     ) strutilSafeCat( command, "\x83 "  , len );
	if( buttons & PSP_CTRL_LTRIGGER ) strutilSafeCat( command, "L "     , len );
	if( buttons & PSP_CTRL_RTRIGGER ) strutilSafeCat( command, "R "     , len );
	if( buttons & PSP_CTRL_SELECT   ) strutilSafeCat( command, "SELECT ", len );
	if( buttons & PSP_CTRL_START    ) strutilSafeCat( command, "START " , len );
}

static MfMenuRc macroeditor_edit_number( char *title, char *unit, int digits, void *number )
{
	if( ! mfMenuGetNumberIsReady() ){
		MfMenuRc rc = mfMenuGetNumberInit( title, unit, (long *)number, digits );
		if( rc != MR_ENTER ) return MR_BACK;
		
		return MR_CONTINUE;
	} else{
		return mfMenuGetNumber();
	}
}

static MfMenuRc macroeditor_edit_buttons( char *title, char *dummy, int avail, void *button )
{
	if( ! mfMenuGetButtonsIsReady() ){
		MfMenuRc rc = mfMenuGetButtonsInit( title, (unsigned int *)button, (unsigned int)avail );
		if( rc != MR_ENTER ) return MR_BACK;
		
		return MR_CONTINUE;
	} else{
		return mfMenuGetButtons();
	}
}

static MfMenuRc macroeditor_edit_analog( SceCtrlData *pad_data, MacroData *macro )
{
	static int selected = 0;
	MfMenuRc rc = MR_CONTINUE;
	
	if( ! st_submenu.func ){
		gbFillRectRel( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, gbOffsetChar( 25 ), gbOffsetLine( 9 ), MFM_BG_COLOR );
		gbLineRectRel( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, gbOffsetChar( 25 ), gbOffsetLine( 9 ), MFM_TRANSPARENT );
		gbPrint( MACROEDITOR_EDIT_TYPE_POS_X + gbOffsetChar( 1 ), MACROEDITOR_EDIT_TYPE_POS_Y + gbOffsetLine( 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Choose a edit value" );
		switch( mfMenuUniDraw( MACROEDITOR_EDIT_TYPE_POS_X + gbOffsetChar( 3 ), MACROEDITOR_EDIT_TYPE_POS_Y + gbOffsetLine( 3 ), st_edit_analog, MF_ARRAY_NUM( st_edit_analog ), &selected, 0 ) ){
			case MR_CONTINUE:
				break;
			case MR_ENTER:
				st_edit_analog[0].value[1].pointer = memsceMalloc( sizeof( long ) * 2 );
				if( ! st_edit_analog[0].value[1].pointer ){
					//エラー
					st_submenu.func = NULL;
					return MR_BACK;
				}
				st_edit_analog[1].value[1].pointer = ((long *)(st_edit_analog[0].value[1].pointer)) + 1;
				*((long *)(st_edit_analog[0].value[1].pointer)) = MACROMGR_GET_ANALOG_X( macro->data );
				*((long *)(st_edit_analog[1].value[1].pointer)) = MACROMGR_GET_ANALOG_Y( macro->data );
				break;
			case MR_BACK:
				selected = 0;
				rc = MR_BACK;
				break;
		}
	} else{
		rc = ( (MacroeditorEditDialog)( st_submenu.func ) )( MFM_GET_CB_ARG_BY_STR( st_submenu.arg, 1 ), MFM_GET_CB_ARG_BY_STR( st_submenu.arg, 2 ), MFM_GET_CB_ARG_BY_INT( st_submenu.arg, 3 ), MFM_GET_CB_ARG_BY_PTR( st_submenu.arg, 0 ) );
		if( rc != MR_CONTINUE ){
			if( rc == MR_ENTER ){
				macro->data = MACROMGR_SET_ANALOG_XY( *((int *)st_edit_analog[0].value[1].pointer), *((int *)st_edit_analog[1].value[1].pointer) );
			}
			memsceFree( st_edit_analog[0].value[1].pointer );
			st_submenu.func = NULL;
		}
	}
	
	return rc;
}

static MfMenuRc macroeditor_edit_rapid( SceCtrlData *pad_data, MacroData *macro )
{
	static int selected = 0;
	MfMenuRc rc = MR_CONTINUE;
	
	long rdelay, pdelay;
	
	rdelay = MACROMGR_GET_RAPIDRDELAY( macro->sub );
	pdelay = MACROMGR_GET_RAPIDPDELAY( macro->sub );
	
	if( ! st_submenu.func ){
		gbFillRectRel( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, gbOffsetChar( 25 ), gbOffsetLine( 9 ), MFM_BG_COLOR );
		gbLineRectRel( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, gbOffsetChar( 25 ), gbOffsetLine( 9 ), MFM_TRANSPARENT );
		gbPrint( MACROEDITOR_EDIT_TYPE_POS_X + gbOffsetChar( 1 ), MACROEDITOR_EDIT_TYPE_POS_Y + gbOffsetLine( 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Choose a edit value" );
		switch( mfMenuUniDraw( MACROEDITOR_EDIT_TYPE_POS_X + gbOffsetChar( 3 ), MACROEDITOR_EDIT_TYPE_POS_Y + gbOffsetLine( 3 ), st_edit_rapid, MF_ARRAY_NUM( st_edit_rapid ), &selected, 0 ) ){
			case MR_CONTINUE: break;
			case MR_ENTER:
				/* 連射設定を一時保存する変数をメニューテーブルへセット */
				st_edit_rapid[0].value[1].pointer = (void *)&(macro->data);
				st_edit_rapid[1].value[1].pointer = (void *)&pdelay;
				st_edit_rapid[2].value[1].pointer = (void *)&rdelay;
				break;
			case MR_BACK:
				selected = 0;
				rc = MR_BACK;
				break;
		}
	} else{
		rc = ( (MacroeditorEditDialog)( st_submenu.func ) )( MFM_GET_CB_ARG_BY_STR( st_submenu.arg, 1 ), MFM_GET_CB_ARG_BY_STR( st_submenu.arg, 2 ), MFM_GET_CB_ARG_BY_INT( st_submenu.arg, 3 ), MFM_GET_CB_ARG_BY_PTR( st_submenu.arg, 0 ) );
		if( rc != MR_CONTINUE ){
			if( rc == MR_ENTER ){
				macro->sub = MACROMGR_SET_RAPIDDELAY( *((int *)st_edit_rapid[1].value[1].pointer), *((int *)st_edit_rapid[2].value[1].pointer) );
			}
			memsceFree( st_edit_rapid[1].value[1].pointer );
			st_submenu.func = NULL;
		}
	}
	
	return rc;
}

static MfMenuRc macroeditor_edit_data( SceCtrlData *pad_data, MacroData *macro )
{
	MfMenuRc rc = MR_BACK;
	
	if( macro->action == MA_DELAY ){
		rc = macroeditor_edit_number( "Set delay (1sec = 1000ms) ", "ms", 9, (long *)&(macro->data) );
	} else if( macro->action == MA_BUTTONS_PRESS || macro->action == MA_BUTTONS_RELEASE || macro->action == MA_BUTTONS_CHANGE ){
		rc = macroeditor_edit_buttons( "Set new buttons", NULL, MACROEDITOR_AVAIL_BUTTONS, (unsigned int *)&(macro->data) );
	} else if( macro->action == MA_ANALOG_MOVE ){
		rc = macroeditor_edit_analog( pad_data, macro );
	} else if( macro->action == MA_RAPIDFIRE_START ){
		rc = macroeditor_edit_rapid( pad_data, macro );
	} else if( macro->action == MA_RAPIDFIRE_STOP ){
		rc = macroeditor_edit_buttons( "Set new buttons", NULL, MACROEDITOR_AVAIL_BUTTONS, (unsigned int *)&(macro->data) );
	}
	
	return rc;
}

static MfMenuRc macroeditor_ch_type( SceCtrlData *pad_data, MacroData *macro )
{
	static int selected = 0;
	
	gbFillRectRel( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, gbOffsetChar( 25 ), gbOffsetLine( 9 ), MFM_BG_COLOR );
	gbLineRectRel( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, gbOffsetChar( 25 ), gbOffsetLine( 9 ), MFM_TRANSPARENT );
	gbPrint( MACROEDITOR_EDIT_TYPE_POS_X + gbOffsetChar( 1 ), MACROEDITOR_EDIT_TYPE_POS_Y + gbOffsetLine( 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Choose a new action" );
	
	switch( mfMenuUniDraw( MACROEDITOR_EDIT_TYPE_POS_X + gbOffsetChar( 3 ), MACROEDITOR_EDIT_TYPE_POS_Y + gbOffsetLine( 3 ), st_edit_chtype, MF_ARRAY_NUM( st_edit_chtype ), &selected, 0 ) ){
		case MR_CONTINUE: return MR_CONTINUE;
		case MR_ENTER:
			( (void (*)( MacroData*, MacroAction, uint64_t, uint64_t) )( st_callback.func ) )( macro, (unsigned int)MFM_GET_CB_ARG_BY_INT( st_callback.arg, 0 ), 0, 0 );
			selected = 0;
		case MR_BACK:
			selected = 0;
			break;
	}
	
	return MR_BACK;
}

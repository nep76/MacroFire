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


/*
static MfMenuRc     macroeditor_edit_delay( SceCtrlData *pad_data, MacroData *macro );
static MfMenuRc     macroeditor_edit_buttons( SceCtrlData *pad_data, MacroData *macro );
static MfMenuRc     macroeditor_edit_analog( SceCtrlData *pad_data, MacroData *macro );
static MfMenuRc     macroeditor_edit_rapiddelay( SceCtrlData *pad_data, MacroData *macro );
*/
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
	{ MT_CALLBACK, "Delay",           0, &(st_callback), { { .pointer = macromgrCmdInit }, { .integer = MA_DELAY }           } },
	{ MT_CALLBACK, "Buttons press",   0, &(st_callback), { { .pointer = macromgrCmdInit }, { .integer = MA_BUTTONS_PRESS }   } },
	{ MT_CALLBACK, "Buttons release", 0, &(st_callback), { { .pointer = macromgrCmdInit }, { .integer = MA_BUTTONS_RELEASE } } },
	{ MT_CALLBACK, "Buttons change",  0, &(st_callback), { { .pointer = macromgrCmdInit }, { .integer = MA_BUTTONS_CHANGE }  } },
	{ MT_CALLBACK, "Analog move",     0, &(st_callback), { { .pointer = macromgrCmdInit }, { .integer = MA_ANALOG_MOVE }     } },
	{ MT_CALLBACK, "Rapidfire start", 0, &(st_callback), { { .pointer = macromgrCmdInit }, { .integer = MA_RAPIDFIRE_START } } },
	{ MT_CALLBACK, "Rapidfire stop",  0, &(st_callback), { { .pointer = macromgrCmdInit }, { .integer = MA_RAPIDFIRE_STOP }  } }
};

MfMenuCallback st_submenu;
static MfMenuItem st_edit_rapid[] = {
	{ MT_CALLBACK, "Rapidfire buttons",       0, &(st_submenu), { { .pointer = macroeditor_edit_buttons }, { .pointer= NULL }, { .string = "Set rapidfire buttons" }, { .pointer = NULL }, { .integer = MACROEDITOR_AVAIL_BUTTONS } } },
	{ MT_CALLBACK, "Rapidfire press delay",   0, &(st_submenu), { { .pointer = macroeditor_edit_number },  { .pointer= NULL }, { .string = "Set press delay" },       { .string = "times" }, { .integer = 4 } } },
	{ MT_CALLBACK, "Rapidfire release delay", 0, &(st_submenu), { { .pointer = macroeditor_edit_number },  { .pointer= NULL }, { .string = "Set release delay" },     { .string = "times" }, { .integer = 4 } } }
};

static MfMenuItem st_edit_analog[] = {
	{ MT_CALLBACK, "Analog X-coordinate", 0, &(st_submenu), { { .pointer = macroeditor_edit_number }, { .pointer= NULL }, { .string = "Set X-coordinate" }, { .string = "(0-255)" }, { .integer = 3 } } },
	{ MT_CALLBACK, "Analog Y-coordinate", 0, &(st_submenu), { { .pointer = macroeditor_edit_number }, { .pointer= NULL }, { .string = "Set Y-coordinate" }, { .string = "(0-255)" }, { .integer = 3 } } }
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
			blitString( blitOffsetChar( 3 ), blitOffsetLine( y++ ), MFM_TEXT_FCCOLOR, MFM_TEXT_BGCOLOR, command );
		} else{
			blitString( blitOffsetChar( 3 ), blitOffsetLine( y++ ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, command );
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
	
	blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 31 ) + ( BLIT_CHAR_HEIGHT >> 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x80\x82 = Move, \x83\x81 = PageMove, \x85 = Edit data, \x84 = Change type, \x87 = Delete\nL = Insert before, R = Insert after, \x86 = Back, START = Exit" );
	
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
		if( macromgrInsert( MIP_BEFORE, selected_macro ) ) st_macro_all_lines++;
	} else if( pad_data->Buttons & PSP_CTRL_RTRIGGER ){
		if( macromgrInsert( MIP_AFTER,  selected_macro ) ){
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
/*
static MfMenuRc macroeditor_edit_delay( SceCtrlData *pad_data, MacroData *macro )
{
	if( cmndlgGetNumbersGetStatus() == CMNDLG_NONE ){
		CmndlgGetNumbersParams *params = (CmndlgGetNumbersParams *)memsceMalloc( sizeof( CmndlgGetNumbersParams ) );
		if( params ){
			params->data = (CmndlgGetNumbersData *)memsceMalloc( sizeof( CmndlgGetNumbersData ) );
			if( ! params->data ){
				memsceFree( params );
				return MR_BACK;
			}
			
			params->numberOfData        = 1;
			params->selectDataNumber    = 0;
			params->rc                  = 0;
			params->ui.x                = MACROEDITOR_EDIT_WAITMS_POS_X;
			params->ui.y                = MACROEDITOR_EDIT_WAITMS_POS_Y;
			params->ui.fgTextColor      = MFM_TEXT_FGCOLOR;
			params->ui.fcTextColor      = MFM_TEXT_FCCOLOR;
			params->ui.bgTextColor      = MFM_TRANSPARENT;
			params->ui.bgColor          = MFM_BG_COLOR;
			params->ui.borderColor      = MFM_TRANSPARENT;
			params->data->title         = "Set delay (1sec = 1000ms)";
			params->data->unit          = "ms";
			params->data->numSave       = (long *)&(macro->data);
			params->data->numDigits     = 9;
			params->data->selectedPlace = 0;
			
			if( cmndlgGetNumbersStart( params ) ){
				cmndlgGetNumbersShutdownStart();
				memsceFree( params->data );
				memsceFree( params );
				return MR_BACK;
			}
		} else{
			//エラー
		}
	} else{
		CmndlgState state;
		if( cmndlgGetNumbersUpdate() ){
			//エラー
		}
		state = cmndlgGetNumbersGetStatus();
		if( state == CMNDLG_SHUTDOWN ){
			CmndlgGetNumbersParams *params = cmndlgGetNumbersGetParams();
			cmndlgGetNumbersShutdownStart();
			memsceFree( params->data );
			memsceFree( params );
			mfMenuKeyRepeatReset();
			return MR_BACK;
		}
	}
	
	return MR_CONTINUE;
}
*/
/*
static MfMenuRc macroeditor_edit_buttons( SceCtrlData *pad_data, MacroData *macro )
{
	if( cmndlgGetButtonsGetStatus() == CMNDLG_NONE ){
		CmndlgGetButtonsParams *params = (CmndlgGetButtonsParams *)memsceMalloc( sizeof( CmndlgGetButtonsParams ) );
		if( params ){
			params->data = (CmndlgGetButtonsData *)memsceMalloc( sizeof( CmndlgGetButtonsData ) );
			if( ! params->data ){
				memsceFree( params );
				return MR_BACK;
			}
			
			params->numberOfData           = 1;
			params->selectDataNumber       = 0;
			params->rc                     = 0;
			params->ui.x                   = MACROEDITOR_EDIT_WAITMS_POS_X;
			params->ui.y                   = MACROEDITOR_EDIT_WAITMS_POS_Y;
			params->ui.fgTextColor         = MFM_TEXT_FGCOLOR;
			params->ui.fcTextColor         = MFM_TEXT_FCCOLOR;
			params->ui.bgTextColor         = MFM_TRANSPARENT;
			params->ui.bgColor             = MFM_BG_COLOR;
			params->ui.borderColor         = MFM_TRANSPARENT;
			params->data->title            = "Set new buttons";
			params->data->buttonsSave      = (unsigned int *)&(macro->data);
			// 十字キー、START、SELECT、○×□△ボタンとLR
			params->data->buttonsAvailable = (
				PSP_CTRL_UP       | PSP_CTRL_RIGHT  | PSP_CTRL_DOWN     | PSP_CTRL_LEFT     |
				PSP_CTRL_TRIANGLE | PSP_CTRL_CIRCLE | PSP_CTRL_CROSS    | PSP_CTRL_SQUARE   |
				PSP_CTRL_START    | PSP_CTRL_SELECT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER
			);
			
			if( cmndlgGetButtonsStart( params ) ){
				cmndlgGetButtonsShutdownStart();
				memsceFree( params->data );
				memsceFree( params );
				return MR_BACK;
			}
		} else{
			//エラー
		}
	} else{
		CmndlgState state;
		if( cmndlgGetButtonsUpdate() ){
			//エラー
		}
		state = cmndlgGetButtonsGetStatus();
		if( state == CMNDLG_SHUTDOWN ){
			CmndlgGetButtonsParams *params = cmndlgGetButtonsGetParams();
			cmndlgGetButtonsShutdownStart();
			memsceFree( params->data );
			memsceFree( params );
			mfMenuKeyRepeatReset();
			return MR_BACK;
		}
	}
	
	return MR_CONTINUE;
}

static MfMenuRc macroeditor_edit_analog( SceCtrlData *pad_data, MacroData *macro )
{
	if( cmndlgGetNumbersGetStatus() == CMNDLG_NONE ){
		int *x = (int *)memsceMalloc( sizeof( int ) );
		int *y = (int *)memsceMalloc( sizeof( int ) );
		CmndlgGetNumbersParams *params = (CmndlgGetNumbersParams *)memsceMalloc( sizeof( CmndlgGetNumbersParams ) );
		if( params ){
			params->data = (CmndlgGetNumbersData *)memsceMalloc( sizeof( CmndlgGetNumbersData ) * 2 );
			if( ! params->data ){
				memsceFree( x );
				memsceFree( y );
				memsceFree( params );
				return MR_BACK;
			}
			
			*x = MACROMGR_GET_ANALOG_X( macro->data );
			*y = MACROMGR_GET_ANALOG_Y( macro->data );
			
			params->numberOfData        = 2;
			params->selectDataNumber    = 0;
			params->rc                  = 0;
			params->ui.x                = MACROEDITOR_EDIT_WAITMS_POS_X;
			params->ui.y                = MACROEDITOR_EDIT_WAITMS_POS_Y;
			params->ui.fgTextColor      = MFM_TEXT_FGCOLOR;
			params->ui.fcTextColor      = MFM_TEXT_FCCOLOR;
			params->ui.bgTextColor      = MFM_TRANSPARENT;
			params->ui.bgColor          = MFM_BG_COLOR;
			params->ui.borderColor      = MFM_TRANSPARENT;
			params->data[0].title         = "Set analog X-coordinate";
			params->data[0].unit          = "(0-255)";
			params->data[0].numSave       = (long *)x;
			params->data[0].numDigits     = 3;
			params->data[0].selectedPlace = 0;
			params->data[1].title         = "Set analog Y-coordinate";
			params->data[1].unit          = "(0-255)";
			params->data[1].numSave       = (long *)y;
			params->data[1].numDigits     = 3;
			params->data[1].selectedPlace = 0;
			
			if( cmndlgGetNumbersStart( params ) ){
				cmndlgGetNumbersShutdownStart();
				memsceFree( x );
				memsceFree( y );
				memsceFree( params->data );
				memsceFree( params );
				return MR_BACK;
			}
		} else{
			//エラー
		}
	} else{
		CmndlgState state;
		if( cmndlgGetNumbersUpdate() ){
			//エラー
		}
		state = cmndlgGetNumbersGetStatus();
		if( state == CMNDLG_SHUTDOWN ){
			CmndlgGetNumbersParams *params = cmndlgGetNumbersGetParams();
			cmndlgGetNumbersShutdownStart();
			if( *((int *)params->data[0].numSave) > 255 ) *((int *)params->data[0].numSave) = 255;
			if( *((int *)params->data[1].numSave) > 255 ) *((int *)params->data[1].numSave) = 255;
			macro->data = MACROMGR_SET_ANALOG_XY( *((int *)params->data[0].numSave), *((int *)params->data[1].numSave) );
			memsceFree( params->data[0].numSave );
			memsceFree( params->data[1].numSave );
			memsceFree( params->data );
			memsceFree( params );
			mfMenuKeyRepeatReset();
			return MR_BACK;
		}
	}
	
	return MR_CONTINUE;
}

static MfMenuRc macroeditor_edit_rapiddelay( SceCtrlData *pad_data, MacroData *macro )
{
	if( cmndlgGetNumbersGetStatus() == CMNDLG_NONE ){
		unsigned int *pd = (unsigned int *)memsceMalloc( sizeof( unsigned int ) );
		unsigned int *rd = (unsigned int *)memsceMalloc( sizeof( unsigned int ) );
		CmndlgGetNumbersParams *params = (CmndlgGetNumbersParams *)memsceMalloc( sizeof( CmndlgGetNumbersParams ) );
		if( params ){
			params->data = (CmndlgGetNumbersData *)memsceMalloc( sizeof( CmndlgGetNumbersData ) * 2 );
			if( ! params->data ){
				memsceFree( pd );
				memsceFree( rd );
				memsceFree( params );
				return MR_BACK;
			}
			
			*pd = MACROMGR_GET_RAPIDPDELAY( macro->sub );
			*rd = MACROMGR_GET_RAPIDRDELAY( macro->sub );
			
			params->numberOfData        = 2;
			params->selectDataNumber    = 0;
			params->rc                  = 0;
			params->ui.x                = MACROEDITOR_EDIT_WAITMS_POS_X;
			params->ui.y                = MACROEDITOR_EDIT_WAITMS_POS_Y;
			params->ui.fgTextColor      = MFM_TEXT_FGCOLOR;
			params->ui.fcTextColor      = MFM_TEXT_FCCOLOR;
			params->ui.bgTextColor      = MFM_TRANSPARENT;
			params->ui.bgColor          = MFM_BG_COLOR;
			params->ui.borderColor      = MFM_TRANSPARENT;
			params->data[0].title         = "Set press delay";
			params->data[0].unit          = "ms";
			params->data[0].numSave       = (long *)pd;
			params->data[0].numDigits     = 9;
			params->data[0].selectedPlace = 0;
			params->data[1].title         = "Set release delay";
			params->data[1].unit          = "ms";
			params->data[1].numSave       = (long *)rd;
			params->data[1].numDigits     = 9;
			params->data[1].selectedPlace = 0;
			
			if( cmndlgGetNumbersStart( params ) ){
				cmndlgGetNumbersShutdownStart();
				memsceFree( pd );
				memsceFree( rd );
				memsceFree( params->data );
				memsceFree( params );
				return MR_BACK;
			}
		} else{
			//エラー
		}
	} else{
		CmndlgState state;
		if( cmndlgGetNumbersUpdate() ){
			//エラー
		}
		state = cmndlgGetNumbersGetStatus();
		if( state == CMNDLG_SHUTDOWN ){
			CmndlgGetNumbersParams *params = cmndlgGetNumbersGetParams();
			cmndlgGetNumbersShutdownStart();
			macro->sub = MACROMGR_SET_RAPIDDELAY( *((unsigned int *)params->data[0].numSave), *((unsigned int *)params->data[1].numSave) );
			memsceFree( params->data[0].numSave );
			memsceFree( params->data[1].numSave );
			memsceFree( params->data );
			memsceFree( params );
			mfMenuKeyRepeatReset();
			return MR_BACK;
		}
	}
	
	return MR_CONTINUE;
}
*/

static MfMenuRc macroeditor_edit_analog( SceCtrlData *pad_data, MacroData *macro )
{
	static int selected = 0;
	MfMenuRc rc = MR_CONTINUE;
	
	if( ! st_submenu.func ){
		blitFillBox( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, blitMeasureChar( 25 ), blitMeasureLine( 9 ), MFM_BG_COLOR );
		blitLineBox( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, blitMeasureChar( 25 ), blitMeasureLine( 9 ), MFM_TRANSPARENT );
		blitString( MACROEDITOR_EDIT_TYPE_POS_X + blitOffsetChar( 1 ), MACROEDITOR_EDIT_TYPE_POS_Y + blitOffsetLine( 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Choose a edit value" );
		switch( mfMenuUniDraw( MACROEDITOR_EDIT_TYPE_POS_X + blitOffsetChar( 3 ), MACROEDITOR_EDIT_TYPE_POS_Y + blitOffsetLine( 3 ), st_edit_analog, MF_ARRAY_NUM( st_edit_analog ), &selected, 0 ) ){
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
	
	if( ! st_submenu.func ){
		blitFillBox( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, blitMeasureChar( 25 ), blitMeasureLine( 9 ), MFM_BG_COLOR );
		blitLineBox( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, blitMeasureChar( 25 ), blitMeasureLine( 9 ), MFM_TRANSPARENT );
		blitString( MACROEDITOR_EDIT_TYPE_POS_X + blitOffsetChar( 1 ), MACROEDITOR_EDIT_TYPE_POS_Y + blitOffsetLine( 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Choose a edit value" );
		switch( mfMenuUniDraw( MACROEDITOR_EDIT_TYPE_POS_X + blitOffsetChar( 3 ), MACROEDITOR_EDIT_TYPE_POS_Y + blitOffsetLine( 3 ), st_edit_rapid, MF_ARRAY_NUM( st_edit_rapid ), &selected, 0 ) ){
			case MR_CONTINUE: break;
			case MR_ENTER:
				st_edit_rapid[1].value[1].pointer = (long *)memsceMalloc( sizeof( long ) * 2 );
				if( ! st_edit_rapid[1].value[1].pointer ){
					//エラー
					st_submenu.func = NULL;
					return MR_BACK;
				}
				st_edit_rapid[2].value[1].pointer = ((long *)st_edit_rapid[1].value[1].pointer) + 1;
				st_edit_rapid[0].value[1].pointer = &(macro->data);
				*((long *)(st_edit_rapid[1].value[1].pointer)) = MACROMGR_GET_RAPIDPDELAY( macro->sub );
				*((long *)(st_edit_rapid[2].value[1].pointer)) = MACROMGR_GET_RAPIDRDELAY( macro->sub );
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
	
	blitFillBox( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, blitMeasureChar( 25 ), blitMeasureLine( 9 ), MFM_BG_COLOR );
	blitLineBox( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, blitMeasureChar( 25 ), blitMeasureLine( 9 ), MFM_TRANSPARENT );
	blitString( MACROEDITOR_EDIT_TYPE_POS_X + blitOffsetChar( 1 ), MACROEDITOR_EDIT_TYPE_POS_Y + blitOffsetLine( 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Choose a new action" );
	
	switch( mfMenuUniDraw( MACROEDITOR_EDIT_TYPE_POS_X + blitOffsetChar( 3 ), MACROEDITOR_EDIT_TYPE_POS_Y + blitOffsetLine( 3 ), st_edit_chtype, MF_ARRAY_NUM( st_edit_chtype ), &selected, 0 ) ){
		case MR_CONTINUE: return MR_CONTINUE;
		case MR_ENTER:
			( (MacromgrCmdInitializer)( st_callback.func ) )( macro, (unsigned int)MFM_GET_CB_ARG_BY_INT( st_callback.arg, 0 ), 0, 0 );
			selected = 0;
		case MR_BACK:
			selected = 0;
			break;
	}
	
	return MR_BACK;
}

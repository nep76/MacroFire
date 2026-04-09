/*
	macroeditor.c
*/

#include "macroeditor.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static unsigned int macroeditor_count_macro_lines  ( MacroData *macro );
static void         macroeditor_print_button_symbol( unsigned int buttons, char *command, size_t len );
static MfMenuRc     macroeditor_edit_data          ( SceCtrlData *pad_data, MacroData *macro );
static MfMenuRc     macroeditor_ch_type            ( SceCtrlData *pad_data, MacroData *macro );

/*-----------------------------------------------
	ローカル変数と定数
-----------------------------------------------*/
static int st_macro_all_lines    = -1;
static int st_selected_macro_num = 0;

#define MACROEDITOR_WAITMS_DIGIT 9
char st_edit_waitms[MACROEDITOR_WAITMS_DIGIT + 1]; /* 999,999,999ミリ秒まで */

#define MACROEDITOR_COORD_DIGIT 3
char st_edit_coord_x[MACROEDITOR_COORD_DIGIT + 1];
char st_edit_coord_y[MACROEDITOR_COORD_DIGIT + 1];

#define MACROEDITOR_TYPE_DELAY           0
#define MACROEDITOR_TYPE_BUTTONS_PRESS   1
#define MACROEDITOR_TYPE_BUTTONS_RELEASE 2
#define MACROEDITOR_TYPE_BUTTONS_CHANGE  3
#define MACROEDITOR_TYPE_ANALOG_MOVE     4
static MfMenuItem st_edit_chtype[] = {
	{ MT_ANCHOR, "Delay",           0, NULL },
	{ MT_ANCHOR, "Buttons press",   0, NULL },
	{ MT_ANCHOR, "Buttons release", 0, NULL },
	{ MT_ANCHOR, "Buttons change",  0, NULL },
	{ MT_ANCHOR, "Analog move",     0, NULL }
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
			snprintf( command, sizeof( command ), "%03d: Delay -----------> %llu ms", line_number, (uint64_t)macro->data );
		} else if( macro->action == MA_BUTTONS_PRESS ){
			snprintf( command, sizeof( command ), "%03d: Buttons press ---> ", line_number );
			macroeditor_print_button_symbol( macro->data, command, 255 );
		} else if( macro->action == MA_BUTTONS_RELEASE ){
			snprintf( command, sizeof( command ), "%03d: Buttons release -> ", line_number );
			macroeditor_print_button_symbol( macro->data, command, 255 );
		} else if( macro->action == MA_BUTTONS_CHANGE ){
			snprintf( command, sizeof( command ), "%03d: Buttons change --> ", line_number );
			macroeditor_print_button_symbol( macro->data, command, 255 );
		} else if( macro->action == MA_ANALOG_MOVE ){
			snprintf( command, sizeof( command ), "%03d: Analog(x,y)------> %d, %d", line_number, (int)MACROMGR_GET_ANALOG_X( macro->data ), (int)MACROMGR_GET_ANALOG_Y( macro->data ) );
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
			case MR_ENTER: /* 無視 */
			case MR_CONTINUE:
				break;
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
			params->data->title            = "Set new buttons state";
			params->data->buttonsSave      = (unsigned int *)&(macro->data);
			/* 十字キー、START、SELECT、○×□△ボタンとLR */
			params->data->buttonsAvailable = 0x0000FFFF;
			
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

static MfMenuRc macroeditor_edit_data( SceCtrlData *pad_data, MacroData *macro )
{
	MfMenuRc rc = MR_BACK;
	
	if( macro->action == MA_DELAY ){
		rc = macroeditor_edit_delay( pad_data, macro );
	} else if( macro->action == MA_BUTTONS_PRESS || macro->action == MA_BUTTONS_RELEASE || macro->action == MA_BUTTONS_CHANGE ){
		rc = macroeditor_edit_buttons( pad_data, macro );
	} else if( macro->action == MA_ANALOG_MOVE ){
		rc = macroeditor_edit_analog( pad_data, macro );
	}
	
	return rc;
}

static MfMenuRc macroeditor_ch_type( SceCtrlData *pad_data, MacroData *macro )
{
	static int selected = 0;
	
	blitFillBox( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, blitMeasureChar( 21 ), blitMeasureLine( 9 ), MFM_BG_COLOR );
	blitLineBox( MACROEDITOR_EDIT_TYPE_POS_X, MACROEDITOR_EDIT_TYPE_POS_Y, blitMeasureChar( 21 ), blitMeasureLine( 9 ), MFM_TRANSPARENT );
	blitString( MACROEDITOR_EDIT_TYPE_POS_X + blitOffsetChar( 1 ), MACROEDITOR_EDIT_TYPE_POS_Y + blitOffsetLine( 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Choose a new action" );
	
	switch( mfMenuUniDraw( MACROEDITOR_EDIT_TYPE_POS_X + blitOffsetChar( 3 ), MACROEDITOR_EDIT_TYPE_POS_Y + blitOffsetLine( 3 ), st_edit_chtype, MF_ARRAY_NUM( st_edit_chtype ), &selected, 0 ) ){
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
				case MACROEDITOR_TYPE_ANALOG_MOVE:
					macro->action = MA_ANALOG_MOVE;
					macro->data   = MACROMGR_ANALOG_NEUTRAL;
					break;
			}
			selected = 0;
		case MR_BACK:
			selected = 0;
			break;
	}
	
	return MR_BACK;
}

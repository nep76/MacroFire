/*
	sosk.c
*/

#include "sosk.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static void cmndlg_sosk_draw_ui( CmndlgSoskParams *params );
static int  cmndlg_sosk_move_focus( int cp, int mv, int max );
static void cmndlg_sosk_cursor_forward( struct cmndlg_sosk_tempdata *tempdatam );
static void cmndlg_sosk_cursor_back( struct cmndlg_sosk_tempdata *tempdata );
static bool cmndlg_sosk_input_add( struct cmndlg_sosk_tempdata *tempdata, char *str, size_t max, char chr );
static bool cmndlg_sosk_input_bs( struct cmndlg_sosk_tempdata *tempdata, char *str );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static CmndlgSoskParams *st_params;
static CtrlpadParams   st_cp_params;
static bool            st_show_usage = false;

static char st_char_table[CMNDLG_SOSK_ROWS][CMNDLG_SOSK_COLS] = {
	{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M' },
	{ 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' },
	{ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm' },
	{ 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' },
	{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '-', '*' },
	{ '!', '?', '@', '#', '$', '%', '&', '<', '=', '>', '~', '^', '/' },
	{ '\"','\'',',', '.', ':', ';', '`', '|', '(', ')', '[', ']', '\\'},
	{ '{', '}', '_',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 }
};

/*=============================================*/

CmndlgSoskParams *cmndlgSoskGetParams( void )
{
	return st_params;
}

CmndlgState cmndlgSoskGetStatus( void )
{
	if( ! st_params ){
		return CMNDLG_NONE;
	} else{
		return st_params->base.state;
	}
}

int cmndlgSoskStart( CmndlgSoskParams *params )
{
	struct cmndlg_sosk_tempdata *tempdata;
	
	if( st_params || ! params ) return CG_ERROR_INVALID_ARGUMENT;
	
	st_params = params;
	
	st_params->base.state = CMNDLG_INIT;
	
	st_params->base.tempBuffer = memsceMalloc( sizeof( struct cmndlg_sosk_tempdata ) + st_params->textMax );
	if( ! st_params->base.tempBuffer ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	tempdata = st_params->base.tempBuffer;
	tempdata->workText = (char *)( (unsigned int)st_params->base.tempBuffer + sizeof( struct cmndlg_sosk_tempdata ) );
	
	strcpy( tempdata->workText, st_params->text );
	tempdata->currentLength = strlen( st_params->text );
	
	if( tempdata->currentLength ){
		if( tempdata->currentLength < CMNDLG_SOSK_RESULT_WIDTH ) {
			tempdata->textOffset = 0;
			tempdata->cursorPos  = tempdata->currentLength;
		} else{
			tempdata->textOffset = tempdata->currentLength - CMNDLG_SOSK_RESULT_WIDTH;
			tempdata->cursorPos  = CMNDLG_SOSK_RESULT_WIDTH;
		}
	} else{
		tempdata->cursorPos   = 0;
		tempdata->textOffset  = 0;
	}
	tempdata->cx = 0;
	tempdata->cy = 0;
	
	if( st_params->options & CMNDLG_SOSK_DISPLAY_CENTER ){
		st_params->ui.x = ( SCR_WIDTH  - gbOffsetChar( CMNDLG_SOSK_WIDTH  ) ) >> 1;
		st_params->ui.y = ( SCR_HEIGHT - gbOffsetLine( CMNDLG_SOSK_HEIGHT ) ) >> 1;
	}
	
	ctrlpadInit( &st_cp_params );
	ctrlpadSetRepeatButtons( &st_cp_params, PSP_CTRL_UP | PSP_CTRL_RIGHT | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_TRIANGLE | PSP_CTRL_CIRCLE | PSP_CTRL_SQUARE );
	
	st_params->base.state = CMNDLG_VISIBLE;
	
	return 0;
}

int cmndlgSoskUpdate( void )
{
	struct cmndlg_sosk_tempdata *tempdata = st_params->base.tempBuffer;
	SceCtrlData pad_data;
	pad_data.Buttons = ctrlpadGetData( &st_cp_params, &pad_data, CTRLPAD_IGNORE_ANALOG_DIRECTION );
	
	cmndlg_sosk_draw_ui( st_params );
	
	if( st_show_usage ){
		CmndlgState state;
		
		cmndlgMessageUpdate();
		state = cmndlgMessageGetStatus();
		if( state == CMNDLG_SHUTDOWN ){
			CmndlgMessageParams *msg_params = cmndlgMessageGetParams();
			cmndlgMessageShutdownStart();
			memsceFree( msg_params );
			ctrlpadUpdateData( &st_cp_params );
			st_show_usage = false;
		}
		return 0;
	}
	
	/* 共通コントロール */
	if( pad_data.Buttons & PSP_CTRL_SELECT ){
		CmndlgMessageParams *msg_params = (CmndlgMessageParams *)memsceMalloc( sizeof( CmndlgMessageParams ) );
		if( msg_params ){
			strutilSafeCopy( msg_params->title, "Usage", 64 );
			strutilSafeCopy(
				msg_params->message,
				"\x80\x82\x83\x81 = Move\nLR   = MoveCursor\n\n\x85 = Input\n\x84 = Space\n\x87 = Backspace\n\nSTART = Accept\n\x86     = Cancel",
				512
			);
			msg_params->options        = CMNDLG_MESSAGE_DISPLAY_CENTER;
			msg_params->rc             = 0;
			msg_params->ui.x           = 0;
			msg_params->ui.y           = 0;
			msg_params->ui.fgTextColor = st_params->ui.fgTextColor;
			msg_params->ui.fcTextColor = st_params->ui.fcTextColor;
			msg_params->ui.bgTextColor = st_params->ui.bgTextColor;
			msg_params->ui.bgColor     = 0xdd000000;
			msg_params->ui.borderColor = 0x0;
			
			if( cmndlgMessageStart( msg_params ) ){
				memsceFree( msg_params );
			}
			st_show_usage = true;
		}
	} else if( pad_data.Buttons & PSP_CTRL_CROSS ){
		st_params->rc         = CMNDLG_CANCEL;
		st_params->base.state = CMNDLG_SHUTDOWN;
	} else if( pad_data.Buttons & PSP_CTRL_START ){
		strcpy( st_params->text, tempdata->workText );
		st_params->rc         = CMNDLG_ACCEPT;
		st_params->base.state = CMNDLG_SHUTDOWN;
	} else if( pad_data.Buttons & PSP_CTRL_CIRCLE ){
		cmndlg_sosk_input_add( tempdata, tempdata->workText, st_params->textMax, st_char_table[tempdata->cy][tempdata->cx] );
		cmndlg_sosk_cursor_forward( tempdata );
	} else if( pad_data.Buttons & PSP_CTRL_TRIANGLE ){
		cmndlg_sosk_input_add( tempdata, tempdata->workText, st_params->textMax, '\x20' );
		cmndlg_sosk_cursor_forward( tempdata );
	} else if( pad_data.Buttons & PSP_CTRL_SQUARE ){
		cmndlg_sosk_input_bs( tempdata, tempdata->workText );
		cmndlg_sosk_cursor_back( tempdata );
	} else if( pad_data.Buttons & PSP_CTRL_UP ){
		do{
			tempdata->cy = cmndlg_sosk_move_focus( tempdata->cy, -1, CMNDLG_SOSK_ROWS - 1 );
		} while( st_char_table[tempdata->cy][tempdata->cx] == 0 );
	} else if( pad_data.Buttons & PSP_CTRL_DOWN ){
		do{
			tempdata->cy = cmndlg_sosk_move_focus( tempdata->cy,  1, CMNDLG_SOSK_ROWS - 1 );
		} while( st_char_table[tempdata->cy][tempdata->cx] == 0 );
	} else if( pad_data.Buttons & PSP_CTRL_LEFT ){
		do{
			tempdata->cx = cmndlg_sosk_move_focus( tempdata->cx, -1, CMNDLG_SOSK_COLS - 1 );
		} while( st_char_table[tempdata->cy][tempdata->cx] == 0 );
	} else if( pad_data.Buttons & PSP_CTRL_RIGHT ){
		do{
			tempdata->cx = cmndlg_sosk_move_focus( tempdata->cx,  1, CMNDLG_SOSK_COLS - 1 );
		} while( st_char_table[tempdata->cy][tempdata->cx] == 0 );
	} else if( pad_data.Buttons & PSP_CTRL_LTRIGGER ){
		cmndlg_sosk_cursor_back( tempdata );
	} else if( pad_data.Buttons & PSP_CTRL_RTRIGGER ){
		cmndlg_sosk_cursor_forward( tempdata );
	}
	
	return 0;
}

int cmndlgSoskShutdownStart( void )
{
	if( st_params->base.state != CMNDLG_SHUTDOWN ){
		st_params->rc         = CMNDLG_CANCEL;
		st_params->base.state = CMNDLG_SHUTDOWN;
	}
	
	memsceFree( st_params->base.tempBuffer );
	
	st_params = NULL;
	ctrlpadReset( &st_cp_params );
	
	return 0;
}

static void cmndlg_sosk_draw_ui( CmndlgSoskParams *params )
{
	char outtext[CMNDLG_SOSK_RESULT_WIDTH + 1];
	int ix, iy;
	struct cmndlg_sosk_tempdata *tempdata = params->base.tempBuffer;
	
	strutilSafeCopy( outtext, tempdata->workText + tempdata->textOffset, CMNDLG_SOSK_RESULT_WIDTH + 1 );
	
	gbFillRectRel( params->ui.x, params->ui.y, gbOffsetChar( CMNDLG_SOSK_WIDTH ), gbOffsetLine( CMNDLG_SOSK_HEIGHT ), params->ui.bgColor );
	gbLineRectRel( params->ui.x, params->ui.y, gbOffsetChar( CMNDLG_SOSK_WIDTH ), gbOffsetLine( CMNDLG_SOSK_HEIGHT ), params->ui.borderColor );
	
	gbPrint(
		params->ui.x + gbOffsetChar( 1 ),
		params->ui.y + gbOffsetLine( CMNDLG_SOSK_HEIGHT - 2 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		"SELECT: Usage"
	);
	
	/* 文字の残りを表示 */
	if( tempdata->textOffset ){
		gbPrint(
			params->ui.x + gbOffsetChar( 1 ),
			params->ui.y + gbOffsetLine( 1 ),
			params->ui.fgTextColor,
			params->ui.bgTextColor,
			"\x83"
		);
	}
	if( tempdata->currentLength > CMNDLG_SOSK_RESULT_WIDTH && tempdata->textOffset < tempdata->currentLength - CMNDLG_SOSK_RESULT_WIDTH ){
		gbPrint(
			params->ui.x + gbOffsetChar( 4 + CMNDLG_SOSK_RESULT_WIDTH ),
			params->ui.y + gbOffsetLine( 1 ),
			params->ui.fgTextColor,
			params->ui.bgTextColor,
			"\x81"
		);
	}
	
	/* 入力欄表示 */
	gbLineRel( 
		params->ui.x + gbOffsetChar( 3 ),
		params->ui.y + gbOffsetLine( 2 ) + 1,
		gbOffsetChar( CMNDLG_SOSK_RESULT_WIDTH ),
		0,
		params->ui.fgTextColor
	);
	
	/* 文字列表示 */
	gbPrint(
		params->ui.x + gbOffsetChar( 3 ),
		params->ui.y + gbOffsetLine( 1 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		outtext
	);
	
	/* カーソル表示 */
	
	gbPrint(
		params->ui.x + gbOffsetChar( 3 + tempdata->cursorPos ),
		params->ui.y + gbOffsetLine( 1 ),
		params->ui.fcTextColor,
		params->ui.bgTextColor,
		"_"
	);
	
	/* 文字数表示 */
	gbPrintf(
		params->ui.x + gbOffsetChar( 1 ),
		params->ui.y + gbOffsetLine( 2 ) + ( GB_CHAR_HEIGHT >> 1 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		"( %d / %d )",
		tempdata->currentLength,
		params->textMax
	);
	
	for( iy = 0; iy < CMNDLG_SOSK_ROWS; iy++ ){
		for( ix = 0; ix < CMNDLG_SOSK_COLS; ix++ ){
			gbPutChar(
				params->ui.x + gbOffsetChar( 6 + ix * 2 ),
				params->ui.y + gbOffsetLine( 4 + iy * 2 ),
				( iy == tempdata->cy && ix == tempdata->cx ? params->ui.fcTextColor : params->ui.fgTextColor ),
				params->ui.bgTextColor,
				st_char_table[iy][ix]
			);
		}
	}
}

static int cmndlg_sosk_move_focus( int cp, int mv, int max )
{
	cp += mv;
	if( cp < 0 ){
		cp = max;
	} else if( cp > max ){
		cp = 0;
	}
	
	return cp;
}


static void cmndlg_sosk_cursor_forward( struct cmndlg_sosk_tempdata *tempdata )
{
	if( tempdata->textOffset + tempdata->cursorPos >= tempdata->currentLength ) return;
	
	tempdata->cursorPos += 1;
	if( tempdata->cursorPos > CMNDLG_SOSK_RESULT_WIDTH - 1 ){
		tempdata->cursorPos = CMNDLG_SOSK_RESULT_WIDTH - 1;
		tempdata->textOffset += 1;
	}
}

static void cmndlg_sosk_cursor_back( struct cmndlg_sosk_tempdata *tempdata )
{
	if( tempdata->textOffset + tempdata->cursorPos <= 0 ) return;
	
	tempdata->cursorPos -= 1;
	if( tempdata->cursorPos < 0 ){
		tempdata->cursorPos = 0;
		if( tempdata->textOffset ) tempdata->textOffset -= 1;
	}
}

static bool cmndlg_sosk_input_add( struct cmndlg_sosk_tempdata *tempdata, char *str, size_t max, char chr )
{
	int offset = tempdata->textOffset + tempdata->cursorPos;
	
	if( tempdata->currentLength + 1 > max ) return false;
	
	if( tempdata->textOffset != tempdata->currentLength ){
		memmove( str + offset + 1, str + offset, tempdata->currentLength - offset + 1 );
	} else{
		str[offset + 1] = '\0';
	}
	str[offset] = chr;
	
	tempdata->currentLength++;
	
	return true;
}

static bool cmndlg_sosk_input_bs( struct cmndlg_sosk_tempdata *tempdata, char *str )
{
	int offset = tempdata->textOffset + tempdata->cursorPos;
	
	if( ! tempdata->currentLength || ! offset ) return false;
	
	if( offset != tempdata->currentLength ){
		memmove( str + offset - 1, str + offset, tempdata->currentLength - offset + 1 );
	} else{
		str[offset - 1] = '\0';
	}
	
	tempdata->currentLength--;
	
	return true;
}

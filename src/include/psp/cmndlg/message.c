/*
	message.c
*/

#include "message.h"


/*-----------------------------------------------
	ƒ[ƒJƒ‹ŠÖ”
-----------------------------------------------*/
static void cmndlg_message_count_width_and_lines( char *str, int *width, int *lines );
static void cmndlg_message_draw_ui( int width, int lines, CmndlgMessageParams *params );

/*-----------------------------------------------
	ƒ[ƒJƒ‹•Ï”
-----------------------------------------------*/
static CmndlgMessageParams *st_params;
static CtrlpadParams       st_cp_params;
static int                 st_width;
static int                 st_lines;
static char                *st_prompt;

/*=============================================*/

CmndlgMessageParams *cmndlgMessageGetParams( void )
{
	return st_params;
}

CmndlgState cmndlgMessageGetStatus( void )
{
	if( ! st_params ){
		return CMNDLG_NONE;
	} else{
		return st_params->base.state;
	}
}

int cmndlgMessageStart( CmndlgMessageParams *params )
{
	int len;
	
	if( st_params || ! params ) return CG_ERROR_INVALID_ARGUMENT;
	
	st_params = params;
	
	st_params->base.state = CMNDLG_INIT;
	
	cmndlg_message_count_width_and_lines( st_params->message, &st_width, &st_lines );
	
	if( st_params->options & CMNDLG_MESSAGE_YESNO ){
		st_prompt = CMNDLG_MESSAGE_PROMPT_YESNO;
	} else{
		st_prompt = CMNDLG_MESSAGE_PROMPT_OK;
	}
	
	len = strlen( st_prompt );
	if( st_width < len ) st_width = len;
	
	len = strlen( st_params->title );
	if( st_width < len ) st_width = len;
	
	/* ŒÅ’ès‚ÆŒÅ’è•‚ð‰ÁŽZ */
	st_width += 2;
	st_lines += 6;
	
	if( st_params->options & CMNDLG_MESSAGE_DISPLAY_CENTER ){
		st_params->ui.x = gbOffsetChar( ( CMNDLG_MESSAGE_MAX_WIDTH  >> 1 ) - ( st_width >> 1 ) );
		st_params->ui.y = gbOffsetLine( ( CMNDLG_MESSAGE_MAX_HEIGHT >> 1 ) - ( st_lines >> 1 ) );
	}
	
	ctrlpadInit( &st_cp_params );
	ctrlpadSetRepeatButtons( &st_cp_params, PSP_CTRL_UP | PSP_CTRL_RIGHT | PSP_CTRL_DOWN | PSP_CTRL_LEFT );
	ctrlpadSetRemap( &st_cp_params, cmndlgGetAlternativeButtonsList(), cmndlgGetAlternativeButtonsListCount() );
	ctrlpadUpdateData( &st_cp_params );
	
	st_params->base.state = CMNDLG_VISIBLE;
	
	return 0;
}

int cmndlgMessageUpdate( void )
{
	SceCtrlData pad_data;
	pad_data.Buttons = ctrlpadGetData( &st_cp_params, &pad_data, CTRLPAD_IGNORE_ANALOG_DIRECTION );
	
	cmndlg_message_draw_ui( st_width, st_lines, st_params );
	
	if( pad_data.Buttons & PSP_CTRL_CIRCLE ){
		st_params->rc         = CMNDLG_ACCEPT;
		st_params->base.state = CMNDLG_SHUTDOWN;
	} else if( ( st_params->options & CMNDLG_MESSAGE_YESNO ) && ( pad_data.Buttons & PSP_CTRL_CROSS ) ){
		st_params->rc         = CMNDLG_CANCEL;
		st_params->base.state = CMNDLG_SHUTDOWN;
	}
	
	return 0;
}

int cmndlgMessageShutdownStart( void )
{
	if( st_params->base.state != CMNDLG_SHUTDOWN ){
		st_params->rc         = CMNDLG_CANCEL;
		st_params->base.state = CMNDLG_SHUTDOWN;
	}
	
	st_params = NULL;
	ctrlpadReset( &st_cp_params );
	
	return 0;
}

static void cmndlg_message_count_width_and_lines( char *str, int *width, int *lines )
{
	int cur_width = 0;
	char *start = str;
	char *end   = NULL;
	
	*width = 0;
	*lines = 1;
	
	while( ( end = strchr( start, '\n' ) ) ){
		*lines += 1;
		cur_width = end - start;
		
		if( cur_width > *width ) *width = cur_width;
		
		start = end + 1;
	}
	
	if( ! *width ) *width = strlen( str );
}

static void cmndlg_message_draw_ui( int width, int lines, CmndlgMessageParams *params )
{
	/* ˜g‚ð•`‰æ */
	gbFillRectRel( params->ui.x, params->ui.y, gbOffsetChar( width ), gbOffsetLine( lines ), params->ui.bgColor );
	gbLineRectRel( params->ui.x, params->ui.y, gbOffsetChar( width ), gbOffsetLine( lines ), params->ui.borderColor );
	
	/* ƒ^ƒCƒgƒ‹•`‰æ */
	gbPrint(
		params->ui.x + gbOffsetChar( ( width ) >> 1 ) - gbOffsetChar( strlen( params->title ) >> 1 ),
		params->ui.y + gbOffsetLine( 1 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		params->title
	);
	
	gbPrint(
		params->ui.x + gbOffsetChar( 1 ),
		params->ui.y + gbOffsetLine( 3 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		params->message
	);
	
	gbPrint(
		params->ui.x + gbOffsetChar( ( width ) >> 1 ) - gbOffsetChar( strlen( st_prompt ) >> 1 ),
		params->ui.y + gbOffsetLine( lines - 2 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		st_prompt
	);
}

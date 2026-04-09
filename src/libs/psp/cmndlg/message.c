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
	
	if( st_params || ! params ) return -1;
	
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
		st_params->ui.x = blitOffsetChar( ( CMNDLG_MESSAGE_MAX_WIDTH  >> 1 ) - ( st_width >> 1 ) );
		st_params->ui.y = blitOffsetLine( ( CMNDLG_MESSAGE_MAX_HEIGHT >> 1 ) - ( st_lines >> 1 ) );
	}
	
	ctrlpadInit( &st_cp_params );
	ctrlpadSetRepeatButtons( &st_cp_params, PSP_CTRL_UP | PSP_CTRL_RIGHT | PSP_CTRL_DOWN | PSP_CTRL_LEFT );
	
	st_params->base.state = CMNDLG_VISIBLE;
	
	return 0;
}

int cmndlgMessageUpdate( void )
{
	SceCtrlData pad_data;
	pad_data.Buttons = ctrlpadGetData( &st_cp_params, &pad_data );
	
	cmndlg_message_draw_ui( st_width, st_lines, st_params );
	
	if( pad_data.Buttons & PSP_CTRL_CIRCLE ){
		st_params->rc         = CMNDLG_ACCEPT;
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
	blitFillBox( params->ui.x, params->ui.y, blitOffsetChar( width ), blitOffsetLine( lines ), params->ui.bgColor );
	blitLineBox( params->ui.x, params->ui.y, blitOffsetChar( width ), blitOffsetLine( lines ), params->ui.borderColor );
	
	/* ƒ^ƒCƒgƒ‹•`‰æ */
	blitString(
		params->ui.x + blitOffsetChar( ( width ) >> 1 ) - blitOffsetChar( strlen( params->title ) >> 1 ),
		params->ui.y + blitOffsetLine( 1 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		params->title
	);
	
	blitString(
		params->ui.x + blitOffsetChar( 1 ),
		params->ui.y + blitOffsetLine( 3 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		params->message
	);
	
	blitString(
		params->ui.x + blitOffsetChar( ( width ) >> 1 ) - blitOffsetChar( strlen( st_prompt ) >> 1 ),
		params->ui.y + blitOffsetLine( lines - 2 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		st_prompt
	);
}

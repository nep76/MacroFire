/*
	getnumbers.c
*/

#include "getnumbers.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static void cmndlg_get_numbers_draw_ui( char *num_start, CmndlgGetNumbersParams *params );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static CmndlgGetNumbersParams *st_params;
static CtrlpadParams          st_cp_params;
static bool                   st_show_usage = false;

/*=============================================*/

CmndlgGetNumbersParams *cmndlgGetNumbersGetParams( void )
{
	return st_params;
}

CmndlgState cmndlgGetNumbersGetStatus( void )
{
	if( ! st_params ){
		return CMNDLG_NONE;
	} else{
		return st_params->base.state;
	}
}

int cmndlgGetNumbersStart( CmndlgGetNumbersParams *params )
{
	int i;
	
	if( st_params || ! params ) return CG_ERROR_INVALID_ARGUMENT;
	
	st_params = params;
	
	params->base.state = CMNDLG_INIT;
	
	st_params->base.tempBuffer = memsceMalloc( ( CMNDLG_GET_NUMBERS_MAX_DIGITS + 1 ) * st_params->numberOfData );
	if( ! st_params->base.tempBuffer ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	for( i = 0; i < params->numberOfData; i++ ){
		if( st_params->data[i].numDigits > CMNDLG_GET_NUMBERS_MAX_DIGITS ){
			st_params->data[i].numDigits = CMNDLG_GET_NUMBERS_MAX_DIGITS;
		} else if( st_params->data[i].numDigits <= 0 ){
			return CG_ERROR_INVALID_ARGUMENT;
		}
		
		if( st_params->data[i].selectedPlace > st_params->data[i].numDigits - 1 ){
			st_params->data[i].selectedPlace = st_params->data[i].numDigits - 1;
		} else if( st_params->data[i].selectedPlace <= 0 ){
			st_params->data[i].selectedPlace = 0;
		}
		
		snprintf( &(((char *)(st_params->base.tempBuffer))[( CMNDLG_GET_NUMBERS_MAX_DIGITS + 1 ) * i]), CMNDLG_GET_NUMBERS_MAX_DIGITS + 1, "%09ld", *(st_params->data[i].numSave) );
	}
	
	ctrlpadInit( &st_cp_params );
	ctrlpadSetRepeatButtons( &st_cp_params, PSP_CTRL_UP | PSP_CTRL_RIGHT | PSP_CTRL_DOWN | PSP_CTRL_LEFT );
	ctrlpadSetRemap( &st_cp_params, cmndlgGetAlternativeButtonsList(), cmndlgGetAlternativeButtonsListCount() );
	ctrlpadUpdateData( &st_cp_params );
	
	params->base.state = CMNDLG_VISIBLE;
	
	return 0;
}

int cmndlgGetNumbersUpdate( void )
{
	CmndlgGetNumbersData *selected_data = &(st_params->data[st_params->selectDataNumber]);
	char *buf_start  = &(((char *)(st_params->base.tempBuffer))[( ( CMNDLG_GET_NUMBERS_MAX_DIGITS + 1 ) * st_params->selectDataNumber ) + ( CMNDLG_GET_NUMBERS_MAX_DIGITS - selected_data->numDigits )]);
	SceCtrlData pad_data;
	pad_data.Buttons = ctrlpadGetData( &st_cp_params, &pad_data, CTRLPAD_IGNORE_ANALOG_DIRECTION );
	
	cmndlg_get_numbers_draw_ui( buf_start, st_params );
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
	
	if( pad_data.Buttons & PSP_CTRL_SELECT ){
		CmndlgMessageParams *msg_params = (CmndlgMessageParams *)memsceMalloc( sizeof( CmndlgMessageParams ) );
		if( msg_params ){
			strutilSafeCopy( msg_params->title, "Usage", 64 );
			strutilSafeCopy(
				msg_params->message,
				"\x83\x81 = Move\n\x80\x82 = Change value\n\n\x85 = Accept\n\x86 = Cancel",
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
		st_params->rc = CMNDLG_CANCEL;
		st_params->base.state = CMNDLG_SHUTDOWN;
	} else if( pad_data.Buttons & PSP_CTRL_CIRCLE ){
		int i;
		/* 数値を表す文字列を実際の数値に変換して代入 */
		for( i = 0; i < st_params->numberOfData; i++ ){
			*(st_params->data[i].numSave) = strtol(
				&(((char *)(st_params->base.tempBuffer))[( ( CMNDLG_GET_NUMBERS_MAX_DIGITS + 1 ) * i ) + ( CMNDLG_GET_NUMBERS_MAX_DIGITS - st_params->data[i].numDigits )]),
				NULL,
				10
			);
		}
		st_params->rc = CMNDLG_ACCEPT;
		st_params->base.state = CMNDLG_SHUTDOWN;
	} else if( pad_data.Buttons & PSP_CTRL_RIGHT ){
		if( selected_data->selectedPlace == 0 ){
			selected_data->selectedPlace = selected_data->numDigits - 1;
		} else{
			selected_data->selectedPlace--;
		}
	} else if( pad_data.Buttons & PSP_CTRL_LEFT ){
		if( selected_data->selectedPlace == selected_data->numDigits - 1 ){
			selected_data->selectedPlace = 0;
		} else{
			selected_data->selectedPlace++;
		}
	} else if( pad_data.Buttons & PSP_CTRL_UP ){
		if( buf_start[( selected_data->numDigits - 1 ) - selected_data->selectedPlace] == '9' ){
			buf_start[( selected_data->numDigits - 1 ) - selected_data->selectedPlace] = '0';
		} else{
			buf_start[( selected_data->numDigits - 1 ) - selected_data->selectedPlace]++;
		}
	} else if( pad_data.Buttons & PSP_CTRL_DOWN ){
		if( buf_start[( selected_data->numDigits - 1 ) - selected_data->selectedPlace] == '0' ){
			buf_start[( selected_data->numDigits - 1 ) - selected_data->selectedPlace] = '9';
		} else{
			buf_start[( selected_data->numDigits - 1 ) - selected_data->selectedPlace]--;
		}
	} else if( pad_data.Buttons & PSP_CTRL_LTRIGGER && st_params->selectDataNumber ){
		st_params->selectDataNumber--;
	} else if( pad_data.Buttons & PSP_CTRL_RTRIGGER && ( st_params->selectDataNumber < st_params->numberOfData - 1 ) ){
		st_params->selectDataNumber++;
	}
	
	return 0;
}

int cmndlgGetNumbersShutdownStart( void )
{
	if( st_params->base.state != CMNDLG_SHUTDOWN ){
		st_params->rc         = CMNDLG_CANCEL;
		st_params->base.state = CMNDLG_SHUTDOWN;
	}
	
	memsceFree( st_params->base.tempBuffer );
	st_params->base.tempBuffer = NULL;
	st_params = NULL;
	
	return 0;
}

static void cmndlg_get_numbers_draw_ui( char *buf, CmndlgGetNumbersParams *params )
{
	int i;
	
	if( params->numberOfData > 1 ){
		int switch_info_pos_y = 9;
		
		gbFillRectRel( params->ui.x, params->ui.y, gbOffsetChar( 30 ), gbOffsetLine( 11 ), params->ui.bgColor );
		gbLineRectRel( params->ui.x, params->ui.y, gbOffsetChar( 30 ), gbOffsetLine( 11 ), params->ui.borderColor );
		
		if( params->selectDataNumber ){
			gbPrintf(
				params->ui.x + gbOffsetChar( 1 ),
				params->ui.y + gbOffsetLine( switch_info_pos_y++ ),
				params->ui.fgTextColor,
				params->ui.bgTextColor,
				"L = %s",
				params->data[params->selectDataNumber - 1].title
			);
		}
		
		if( params->selectDataNumber < params->numberOfData - 1 ){
			gbPrintf(
				params->ui.x + gbOffsetChar( 1 ),
				params->ui.y + gbOffsetLine( switch_info_pos_y ),
				params->ui.fgTextColor,
				params->ui.bgTextColor,
				"R = %s",
				params->data[params->selectDataNumber + 1].title
			);
		}
	} else{
		gbFillRectRel( params->ui.x, params->ui.y, gbOffsetChar( 30 ), gbOffsetLine( 8 ), params->ui.bgColor );
		gbLineRectRel( params->ui.x, params->ui.y, gbOffsetChar( 30 ), gbOffsetLine( 8 ), params->ui.borderColor );
	}
	
	gbPrint(
		params->ui.x + gbOffsetChar( 1 ),
		params->ui.y + gbOffsetLine( 1 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		params->data[params->selectDataNumber].title
	);
	gbPrint(
		params->ui.x + gbOffsetChar( 1 ),
		params->ui.y + gbOffsetLine( 6 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		"SELECT: Usage"
	);
	
	for( i = 0; buf[i] != '\0'; i++ ){
		if( ( params->data[params->selectDataNumber].numDigits - 1) - params->data[params->selectDataNumber].selectedPlace == i ){
			char num_reel[] = { '\x80', '\n', buf[i], '\n', '\x82', '\0' };
			gbPrint(
				params->ui.x + gbOffsetChar( i + 1 ),
				params->ui.y + gbOffsetLine( 2 ),
				params->ui.fcTextColor,
				params->ui.bgTextColor,
				num_reel
			);
		} else{
			gbPutChar(
				params->ui.x + gbOffsetChar( i + 1 ),
				params->ui.y + gbOffsetLine( 3 ),
				params->ui.fgTextColor,
				params->ui.bgTextColor,
				buf[i]
			);
		}
	}
	
	gbPrint(
		params->ui.x + gbOffsetChar( i + 2 ),
		params->ui.y + gbOffsetLine( 3 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		params->data[params->selectDataNumber].unit
	);
}

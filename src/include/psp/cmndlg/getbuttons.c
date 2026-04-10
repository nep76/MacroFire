/*
	getbuttons.c
*/

#include "getbuttons.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static void cmndlg_get_buttons_draw_ui( CmndlgGetButtonsParams *params );
static int cmndlg_get_buttons_focus_next( int current_focus );
static int cmndlg_get_buttons_focus_prev( int current_focus );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static CmndlgGetButtonsParams *st_params;
static CtrlpadParams          st_cp_params;
static bool                   st_show_usage = false;
static bool                   st_reload = true;
static int                    st_avail_buttons_num;
static struct cmndlg_get_buttons_buttonstat st_btnstat[] = {
	{ "Circle       ", PSP_CTRL_CIRCLE,   false },
	{ "Cross        ", PSP_CTRL_CROSS,    false },
	{ "Square       ", PSP_CTRL_SQUARE,   false },
	{ "Triangle     ", PSP_CTRL_TRIANGLE, false },
	{ "Up           ", PSP_CTRL_UP,       false },
	{ "Right        ", PSP_CTRL_RIGHT,    false },
	{ "Down         ", PSP_CTRL_DOWN,     false },
	{ "Left         ", PSP_CTRL_LEFT,     false },
	{ "L-trig       ", PSP_CTRL_LTRIGGER, false },
	{ "R-trig       ", PSP_CTRL_RTRIGGER, false },
	{ "SELECT       ", PSP_CTRL_SELECT,   false },
	{ "START        ", PSP_CTRL_START,    false },
	{ "MusicNote(\x88) ", PSP_CTRL_NOTE,     false },
	{ "Brightness   ", PSP_CTRL_SCREEN,   false },
	{ "VolumeUp     ", PSP_CTRL_VOLUP,    false },
	{ "VolumeDown   ", PSP_CTRL_VOLDOWN,  false },
	{ "Hold         ", PSP_CTRL_HOLD,     false },
	{ "WLAN-SwitchUp", PSP_CTRL_WLAN_UP,  false },
	{ "HOME         ", PSP_CTRL_HOME,     false },
	{ "AnalogUp     ", CTRLPAD_CTRL_ANALOG_UP,    false },
	{ "AnalogRight  ", CTRLPAD_CTRL_ANALOG_RIGHT, false },
	{ "AnalogDown   ", CTRLPAD_CTRL_ANALOG_DOWN,  false },
	{ "AnalogLeft   ", CTRLPAD_CTRL_ANALOG_LEFT,  false }
};

/*=============================================*/

CmndlgGetButtonsParams *cmndlgGetButtonsGetParams( void )
{
	return st_params;
}

CmndlgState cmndlgGetButtonsGetStatus( void )
{
	if( ! st_params ){
		return CMNDLG_NONE;
	} else{
		return st_params->base.state;
	}
}

int cmndlgGetButtonsStart( CmndlgGetButtonsParams *params )
{
	int i, j;
	struct cmndlg_get_buttons_tempdata *tempdata;
	
	if( st_params || ! params ) return CG_ERROR_INVALID_ARGUMENT;
	
	st_params = params;
	st_params->base.state = CMNDLG_INIT;
	
	st_params->base.tempBuffer = memsceMalloc( sizeof( struct cmndlg_get_buttons_tempdata ) * st_params->numberOfData );
	if( ! st_params->base.tempBuffer ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	tempdata = st_params->base.tempBuffer;
	for( i = 0; i < st_params->numberOfData; i++ ){
		*(st_params->data[i].buttonsSave) &= st_params->data[i].buttonsAvailable;
		tempdata[i].buttons  = *(st_params->data[i].buttonsSave);
		for( j = 0; j < sizeof( st_btnstat ) / sizeof( st_btnstat[0] ); j++ ){
			if( st_params->data[i].buttonsAvailable & st_btnstat[j].button ){
				tempdata[i].selected = j;
				break;
			}
		}
	}
	
	st_reload = true;
	ctrlpadInit( &st_cp_params );
	ctrlpadSetRepeatButtons( &st_cp_params, PSP_CTRL_UP | PSP_CTRL_RIGHT | PSP_CTRL_DOWN | PSP_CTRL_LEFT );
	
	params->base.state = CMNDLG_VISIBLE;
	
	return 0;
}

int cmndlgGetButtonsUpdate( void )
{
	int i;
	CmndlgGetButtonsData *selected_data          = &(st_params->data[st_params->selectDataNumber]);
	struct cmndlg_get_buttons_tempdata *tempdata = &(((struct cmndlg_get_buttons_tempdata *)(st_params->base.tempBuffer))[st_params->selectDataNumber]);
	SceCtrlData pad_data;
	pad_data.Buttons = ctrlpadGetData( &st_cp_params, &pad_data, CTRLPAD_IGNORE_ANALOG_DIRECTION );
	
	if( st_reload ){
		st_avail_buttons_num = 0;
		for( i = 0; i < sizeof( st_btnstat ) / sizeof( st_btnstat[0] ); i++ ){
			if( selected_data->buttonsAvailable & st_btnstat[i].button ){
				st_avail_buttons_num++;
				st_btnstat[i].available = true;
			} else{
				st_btnstat[i].available = false;
			}
		}
		st_reload = false;
	}
	
	cmndlg_get_buttons_draw_ui( st_params );
	
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
				"\x80\x82 = Move\n\x83\x81 = Switch\n\n\x85  = Accept\n\x86  = Cancel",
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
		/* 一時保存していたボタン情報を実際に代入 */
		for( i = 0; i < st_params->numberOfData; i++ ){
			*(st_params->data[i].buttonsSave) = ((struct cmndlg_get_buttons_tempdata *)(st_params->base.tempBuffer))[i].buttons;
		}
		st_params->rc = CMNDLG_ACCEPT;
		st_params->base.state = CMNDLG_SHUTDOWN;
	} else if( pad_data.Buttons & PSP_CTRL_UP ){
		tempdata->selected = cmndlg_get_buttons_focus_prev( tempdata->selected );
	} else if( pad_data.Buttons & PSP_CTRL_DOWN ){
		tempdata->selected = cmndlg_get_buttons_focus_next( tempdata->selected );
	} else if( pad_data.Buttons & ( PSP_CTRL_LEFT | PSP_CTRL_RIGHT ) ){
		if( tempdata->buttons & st_btnstat[tempdata->selected].button ){
			tempdata->buttons ^= st_btnstat[tempdata->selected].button;
		} else{
			tempdata->buttons |= st_btnstat[tempdata->selected].button;
		}
	} else if( pad_data.Buttons & PSP_CTRL_LTRIGGER && st_params->selectDataNumber ){
		st_params->selectDataNumber--;
		st_reload = true;
	} else if( pad_data.Buttons & PSP_CTRL_RTRIGGER && ( st_params->selectDataNumber < st_params->numberOfData - 1 ) ){
		st_params->selectDataNumber++;
		st_reload = true;
	}
	
	return 0;
}

int cmndlgGetButtonsShutdownStart( void )
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

static void cmndlg_get_buttons_draw_ui( CmndlgGetButtonsParams *params )
{
	int i, l;
	
	if( params->numberOfData > 1 ){
		int switch_info_pos_y = 9;
		
		gbFillRectRel( params->ui.x, params->ui.y, gbOffsetChar( 30 ), gbOffsetLine( 8 + st_avail_buttons_num ), params->ui.bgColor );
		gbLineRectRel( params->ui.x, params->ui.y, gbOffsetChar( 30 ), gbOffsetLine( 8 + st_avail_buttons_num ), params->ui.borderColor );
		
		if( params->selectDataNumber ){
			gbPrintf(
				params->ui.x + gbOffsetChar( 1 ),
				params->ui.y + gbOffsetLine( 7 + st_avail_buttons_num + switch_info_pos_y++ ),
				params->ui.fgTextColor,
				params->ui.bgTextColor,
				"L = %s",
				params->data[params->selectDataNumber - 1].title
			);
		}
		
		if( params->selectDataNumber < params->numberOfData - 1 ){
			gbPrintf(
				params->ui.x + gbOffsetChar( 1 ),
				params->ui.y + gbOffsetLine( 7 + st_avail_buttons_num + switch_info_pos_y ),
				params->ui.fgTextColor,
				params->ui.bgTextColor,
				"R = %s",
				params->data[params->selectDataNumber + 1].title
			);
		}
	} else{
		gbFillRectRel( params->ui.x, params->ui.y, gbOffsetChar( 30 ), gbOffsetLine( 6 + st_avail_buttons_num ), params->ui.bgColor );
		gbLineRectRel( params->ui.x, params->ui.y, gbOffsetChar( 30 ), gbOffsetLine( 6 + st_avail_buttons_num ), params->ui.borderColor );
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
		params->ui.y + gbOffsetLine( 4 + st_avail_buttons_num ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		"SELECT: Usage"
	);
	
	for( i = 0, l = 0; i < sizeof( st_btnstat ) / sizeof( st_btnstat[0] ); i++ ){
		if( st_btnstat[i].available ){
			gbPrintf(
				params->ui.x + gbOffsetChar( 3 ),
				params->ui.y + gbOffsetLine( 3 + l ),
				i == ((struct cmndlg_get_buttons_tempdata *)(params->base.tempBuffer))[params->selectDataNumber].selected ? params->ui.fcTextColor : params->ui.fgTextColor,
				params->ui.bgTextColor,
				"%s: %s",
				st_btnstat[i].name,
				((struct cmndlg_get_buttons_tempdata *)(params->base.tempBuffer))[params->selectDataNumber].buttons & st_btnstat[i].button ? "ON" : "OFF"
			);
			l++;
		}
	}
}

static int cmndlg_get_buttons_focus_next( int current_focus )
{
	do{
		current_focus++;
		if( current_focus >= sizeof( st_btnstat ) / sizeof( st_btnstat[0] ) ){
			current_focus = 0;
		}
	} while( ! st_btnstat[current_focus].available );
	
	return current_focus;
}

static int cmndlg_get_buttons_focus_prev( int current_focus )
{
	do{
		current_focus--;
		if( current_focus < 0 ){
			current_focus = sizeof( st_btnstat ) / sizeof( st_btnstat[0] ) - 1;
		}
	} while( ! st_btnstat[current_focus].available );
	
	return current_focus;
}

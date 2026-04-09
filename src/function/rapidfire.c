/*
	RapidFire
*/

#include "rapidfire.h"

static RapidfireConf st_rfConf[] = {
	{ PSP_CTRL_CIRCLE,   0 },
	{ PSP_CTRL_CROSS,    0 },
	{ PSP_CTRL_SQUARE,   0 },
	{ PSP_CTRL_TRIANGLE, 0 },
	{ PSP_CTRL_UP,       0 },
	{ PSP_CTRL_RIGHT,    0 },
	{ PSP_CTRL_DOWN,     0 },
	{ PSP_CTRL_LEFT,     0 },
	{ PSP_CTRL_LTRIGGER, 0 },
	{ PSP_CTRL_RTRIGGER, 0 },
	{ PSP_CTRL_START,    0 },
	{ PSP_CTRL_SELECT,   0 }
};

#define RAPIDFIRE_MODE_NORMAL     0
#define RAPIDFIRE_MODE_SEMI_RAPID 1
#define RAPIDFIRE_MODE_AUTO_RAPID 2
#define RAPIDFIRE_MODE_AUTO_HOLD  3
static MfMenuItem st_rfMenuTable[] = {
	{ MT_OPTION, &(st_rfConf[0].mode),  "Circle  ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfConf[1].mode),  "Cross   ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfConf[2].mode),  "Square  ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfConf[3].mode),  "Triangle", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_OPTION, &(st_rfConf[4].mode),  "Up      ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfConf[5].mode),  "Right   ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfConf[6].mode),  "Down    ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfConf[7].mode),  "Left    ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_OPTION, &(st_rfConf[8].mode),  "L-trig  ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfConf[9].mode),  "R-trig  ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_OPTION, &(st_rfConf[10].mode), "START   ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfConf[11].mode), "SELECT  ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
};



static short rapid = 0;
static SceCtrlData st_dupe_pad;

void rapidfireMain( HookCaller caller, SceCtrlData *pad_data, void *argp )
{
	int i;
	
	if( caller == CALL_PEEK_BUFFER_NEGATIVE || caller == CALL_READ_BUFFER_NEGATIVE )
		pad_data->Buttons = ~pad_data->Buttons;
	
	memcpy( &st_dupe_pad, pad_data, sizeof( SceCtrlData ) );
	
	if( caller != CALL_PEEK_LATCH && caller != CALL_READ_LATCH ){
		for( i = 0; i < ARRAY_NUM( st_rfConf ); i++ ){
			if(
				(st_dupe_pad.Buttons & st_rfConf[i].button && st_rfConf[i].mode == RAPIDFIRE_MODE_SEMI_RAPID ) ||
				st_rfConf[i].mode == RAPIDFIRE_MODE_AUTO_RAPID
			 ){
				pad_data->Buttons ^= rapid ? st_rfConf[i].button : 0;
			} else if( st_rfConf[i].mode == RAPIDFIRE_MODE_AUTO_HOLD ){
				pad_data->Buttons |= st_rfConf[i].button;
			}
		}
		rapid = ( ! rapid );
	}
	
	if( caller == CALL_PEEK_BUFFER_NEGATIVE || caller == CALL_READ_BUFFER_NEGATIVE )
		pad_data->Buttons = ~pad_data->Buttons;
	
	return;
}

MfMenuReturnCode rapidfireMenu( SceCtrlLatch *pad_latch, SceCtrlData *pad_data, void *argp )
{
	static int selected = 0;
	
	switch( mfMenuVertical( blitOffsetChar( 5 ), blitOffsetLine( 4 ), BLIT_SCR_WIDTH, st_rfMenuTable, ARRAY_NUM( st_rfMenuTable ), &selected ) ){
		case MR_CONTINUE:
			blitString( blitOffsetChar( 3 ), blitOffsetLine(  2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Please choose a rapidfire mode per buttons." );
			blitString( blitOffsetChar( 5 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "NORMAL: standard control mode.\nRAPID: hold the button to rapidfire.\nAUTO-RAPID: always to rapidfire.\nAUTO-HOLD: always to press and hold." );
			blitString( blitOffsetChar( 3 ), blitOffsetLine( 31 ), MENU_FGCOLOR, MENU_BGCOLOR, "\x80\x82 = Move, \x83\x81 = Change mode, \x86 = Back, START = Exit" );
			break;
		case MR_ENTER:
			break;
		case MR_BACK:
			//selected = 0;
			return MR_BACK;
	}
	
	return MR_CONTINUE;
}

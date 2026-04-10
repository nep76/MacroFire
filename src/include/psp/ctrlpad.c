/*
	ctrlpad.c
*/

#include "ctrlpad.h"

/*-----------------------------------------------
	ローカル関数プロトタイプ
-----------------------------------------------*/
static unsigned int ctrlpad_remap_button( unsigned int buttons, CtrlpadAltBtn *altbtn, unsigned int num );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/

/*=============================================*/

void ctrlpadInit( CtrlpadParams *params )
{
	if( ! params ) return;
	
	params->tickRepeatDelayLow  = 250000;
	params->tickRepeatDelayHigh = 100000;
	params->countLowToHigh      = 1;
	params->maskRepeatButtons   = 0;
	
	params->alternativeButtons = NULL;
	params->numberOfEntries    = 0;
	params->useHeap            = false;
	
	sceCtrlSetSamplingMode( PSP_CTRL_MODE_ANALOG );
	
	ctrlpadUpdateData( params );
}

void ctrlpadPref( CtrlpadParams *params, uint64_t low, uint64_t high, int count )
{
	if( ! params ) return;
	
	params->tickRepeatDelayLow  = low;
	params->tickRepeatDelayHigh = high;
	params->countLowToHigh      = count;
}

void ctrlpadReset( CtrlpadParams *params )
{
	if( ! params ) return;
	
	params->tickLast    = 0;
	params->countLow    = 0;
	params->lastButtons = 0;
}

void ctrlpadSetRemap( CtrlpadParams *params, CtrlpadAltBtn *altbtn, unsigned int num )
{
	if( ! params || ! altbtn ) return;
	
	ctrlpadClearRemap( params );
	params->alternativeButtons = altbtn;
	params->numberOfEntries    = num;
}

bool ctrlpadStoreRemap( CtrlpadParams *params, CtrlpadAltBtn *altbtn, unsigned int num )
{
	unsigned int size = sizeof( CtrlpadAltBtn ) * num;
	
	if( ! params || ! altbtn ) return true;
	
	ctrlpadClearRemap( params );
	params->alternativeButtons = memsceMalloc( size );
	if( ! params->alternativeButtons ) return false;
	
	params->numberOfEntries = num;
	params->useHeap         = true;
	
	memcpy( params->alternativeButtons, altbtn, size );
	
	return true;
}

void ctrlpadClearRemap( CtrlpadParams *params )
{
	if( ! params ) return;
	
	if( params->alternativeButtons && params->useHeap ){
		memsceFree( params->alternativeButtons );
	}
	params->alternativeButtons = NULL;
	params->numberOfEntries    = 0;
	params->useHeap            = false;
}

void ctrlpadSetRepeatButtons( CtrlpadParams *params, unsigned int mask )
{
	if( ! params ) return;
	
	params->maskRepeatButtons = mask;
}

void ctrlpadUpdateData( CtrlpadParams *params )
{
	SceCtrlData pad;
	if( ! params ) return;
	
	sceRtcGetCurrentTick( &(params->tickLast) );
	sceCtrlPeekBufferPositive( &pad, 1 );
	
	params->countLow    = 0;
	params->lastButtons = params->alternativeButtons ? ctrlpad_remap_button( pad.Buttons, params->alternativeButtons, params->numberOfEntries ) : pad.Buttons;
}

unsigned int ctrlpadGetData( CtrlpadParams *params,SceCtrlData *pad_data, int analog_deadzone )
{
	uint64_t current_tick;
	uint64_t *delay_tick;
	unsigned int buttons;
	
	sceRtcGetCurrentTick( &current_tick );
	sceCtrlPeekBufferPositive( pad_data, 1 );
	
	buttons = params->alternativeButtons ? ctrlpad_remap_button( pad_data->Buttons, params->alternativeButtons, params->numberOfEntries ) : pad_data->Buttons;
	
	if( analog_deadzone >= 0 ){
		buttons |= padutilGetAnalogDirection( pad_data->Lx, pad_data->Ly, analog_deadzone );
	}
	
	if( params->countLow >= params->countLowToHigh ){
		delay_tick = &(params->tickRepeatDelayHigh);
	} else{
		delay_tick = &(params->tickRepeatDelayLow);
	}
	
	if( buttons ){
		if( buttons == params->lastButtons ){
			if( ! params->tickRepeatDelayLow && ! params->tickRepeatDelayHigh && ! params->countLowToHigh ){
				buttons = 0;
			} else if( current_tick - params->tickLast > *delay_tick ){
				params->tickLast    = current_tick;
				params->lastButtons = buttons;
				if( params->countLow <= params->countLowToHigh ) params->countLow++;
				buttons             &= params->maskRepeatButtons;
			} else{
				buttons = 0;
			}
		} else{
			params->tickLast    = current_tick;
			params->lastButtons = buttons;
			params->countLow    = 0;
		}
	}
	return buttons;
}

static unsigned int ctrlpad_remap_button( unsigned int buttons, CtrlpadAltBtn *altbtn, unsigned int num )
{
	unsigned int i, newbuttons = 0, rmvbuttons = 0;
	
	for( i = 0; i < num; i++ ){
		if( altbtn[i].realButtons && altbtn[i].altButtons ){
			rmvbuttons |= altbtn[i].altButtons;
			if( ( buttons & altbtn[i].realButtons ) == altbtn[i].realButtons ){
				rmvbuttons |= altbtn[i].realButtons;
				newbuttons |= altbtn[i].altButtons;
			}
		}
	}
	buttons &= ~rmvbuttons;
	buttons |= newbuttons;
	
	return buttons;
}

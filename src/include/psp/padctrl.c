/*=========================================================

	padctrl.c

	パッドデータの取得時に、
	アナログスティックの調整や代替ボタンの変換、
	キーリピートの処理を行う。

=========================================================*/
#include "padctrl.h"

/*=========================================================
	ローカル型宣言
=========================================================*/
struct padctrl_params {
	unsigned int buttons;
	uint64_t     lastTick;
	unsigned int lastButtons;
	unsigned int prePressed;
	bool         doRepeat;
};

/*=========================================================
	ローカル関数
=========================================================*/
static void padctrl_proc( PadctrlUID uid, SceCtrlData *pad, int count );

/*=========================================================
	関数
=========================================================*/
PadctrlUID padctrlNew( void )
{
	struct padctrl_params *params = memoryAlloc( sizeof( struct padctrl_params ) );
	if( ! params ) return 0;
	
	sceCtrlSetSamplingMode( PSP_CTRL_MODE_ANALOG );
	
	params->buttons  = 0;
	padctrlResetRepeat( (PadctrlUID)params );
	
	return (PadctrlUID)params;
}

void padctrlDestroy( PadctrlUID uid )
{
	if( uid ) memoryFree( (void *)uid );
}

void padctrlSetRepeatButtons( PadctrlUID uid, PadutilButtons buttons )
{
	if( ! buttons ) return;
	((struct padctrl_params *)uid)->buttons = buttons;
}

unsigned int padctrlReadBuffer( PadctrlUID uid, SceCtrlData *pad, int count )
{
	unsigned int realbuttons;
	
	sceCtrlReadBufferPositive( pad, count );
	realbuttons = pad->Buttons;
	
	padctrl_proc( uid, pad, count );
	return realbuttons;
}

unsigned int padctrlPeekBuffer( PadctrlUID uid, SceCtrlData *pad, int count )
{
	unsigned int realbuttons;
	
	sceCtrlReadBufferPositive( pad, count );
	realbuttons = pad->Buttons;
	
	padctrl_proc( uid, pad, count );
	return realbuttons;
}

void padctrlResetRepeat( PadctrlUID uid )
{
	SceCtrlData pad;
	sceCtrlPeekBufferPositive( &pad, 1 );
	
	((struct padctrl_params *)uid)->lastTick    = 0;
	((struct padctrl_params *)uid)->lastButtons = pad.Buttons;
	((struct padctrl_params *)uid)->prePressed  = pad.Buttons & PADUTIL_PAD_NORMAL_BUTTONS;
	((struct padctrl_params *)uid)->doRepeat    = false;

}

static void padctrl_proc( PadctrlUID uid, SceCtrlData *pad, int count )
{
	struct padctrl_params *params = (struct padctrl_params *)uid;
	uint64_t current_tick;
	uint64_t delay_tick;
	
	if( params->prePressed ){
		if( params->prePressed == ( pad->Buttons & PADUTIL_PAD_NORMAL_BUTTONS ) ){
			pad->Buttons &= ~PADUTIL_PAD_NORMAL_BUTTONS;
		} else{
			params->prePressed = 0;
		}
	}
	
	if( ! pad->Buttons ) return;
	
	sceRtcGetCurrentTick( &current_tick );
	delay_tick = params->doRepeat ? PADCTRL_REPEAT_NEXT_DELAY : PADCTRL_REPEAT_START_DELAY;
	
	if( pad->Buttons == params->lastButtons ){
		if( current_tick - params->lastTick >= delay_tick ){
			unsigned int button, buttons = 0;
			
			params->lastTick = current_tick;
			params->doRepeat = true;
			
			for( button = 1; button; button <<= 1 ){
				if( ( pad->Buttons & button ) && ( params->buttons & button ) ) buttons |= button;
			}
			pad->Buttons = buttons;
		} else{
			pad->Buttons &= PADUTIL_PAD_TOGGLE_BUTTONS;
		}
	} else{
		params->lastTick    = current_tick;
		params->lastButtons = pad->Buttons;
		params->doRepeat    = false;
	}
	
	if( count > 1 ){
		int i;
		for( i = 1; i < count; i++ ){
			memcpy( (void *)&pad[i], (void *)&pad[0], sizeof( SceCtrlData ) );
		}
	}
}

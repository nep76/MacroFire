/*
	Controll pad utility
*/

#ifndef CTRLPAD_H
#define CTRLPAD_H

#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include <stdbool.h>
#include "utils/strutil.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/

/*
	アナログパッドの方向を格納。
	ボタンコードとして使用し、enum PspCtrlButtonsが使用していない空きビットを使用。
*/
enum ctrlpad_analog_direction {
	CTRLPAD_CTRL_ANALOG_UP    = 0x00000002,
	CTRLPAD_CTRL_ANALOG_RIGHT = 0x00000004,
	CTRLPAD_CTRL_ANALOG_DOWN  = 0x00000200,
	CTRLPAD_CTRL_ANALOG_LEFT  = 0x00000400
};

struct ctrlpad_button {
	unsigned int button;
	char *label;
};

typedef struct {
	SceCtrlData  lastPadData;
	uint64_t     tickLast;
	uint64_t     tickRepeatDelayLow;
	uint64_t     tickRepeatDelayHigh;
	int          countLowToHigh;
	int          countLow;
	unsigned int maskRepeatButtons;
} CtrlpadParams;

/*-----------------------------------------------
	関数
-----------------------------------------------*/
void ctrlpadInit( CtrlpadParams *params );
void ctrlpadPref( CtrlpadParams *params, uint64_t low, uint64_t high, int count );
void ctrlpadSetRepeatButtons( CtrlpadParams *params, unsigned int mask );
void ctrlpadReset( CtrlpadParams *params );
void ctrlpadDataClear( SceCtrlData *pad_data );
void ctrlpadUpdateData( CtrlpadParams *params );
unsigned int ctrlpadGetData( CtrlpadParams *params, SceCtrlData *pad_data );

/*-----------------------------------------------
	ユーティリティ関数
-----------------------------------------------*/
char *ctrlpadUtilButtonsToString( unsigned int buttons, char *str, size_t max );
unsigned int ctrlpadUtilStringToButtons( char *str );

#ifdef __cplusplus
}
#endif

#endif

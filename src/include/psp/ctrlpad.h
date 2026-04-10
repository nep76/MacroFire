/*
	Controll pad utility
	
	$NOTICE$
	
	‚Å‚«‚ªˆ«‚¢‚Ì‚Åì‚è’¼‚·B
*/

#ifndef CTRLPAD_H
#define CTRLPAD_H

#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include <stdbool.h>
#include "utils/strutil.h"
#include "psp/padutil.h"

/*-----------------------------------------------
	ƒ}ƒNƒ
-----------------------------------------------*/
#define CTRLPAD_IGNORE_ANALOG_DIRECTION       -1

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	Œ^éŒ¾
-----------------------------------------------*/
typedef struct {
	unsigned int realButtons;
	unsigned int altButtons;
} CtrlpadAltBtn;

typedef struct {
	unsigned int  lastButtons;
	uint64_t      tickLast;
	uint64_t      tickRepeatDelayLow;
	uint64_t      tickRepeatDelayHigh;
	int           countLowToHigh;
	int           countLow;
	unsigned int  maskRepeatButtons;
	
	CtrlpadAltBtn *alternativeButtons;
	unsigned int  numberOfEntries;
	bool          useHeap;
} CtrlpadParams;

/*-----------------------------------------------
	ŠÖ”
-----------------------------------------------*/
void ctrlpadInit( CtrlpadParams *params );
void ctrlpadPref( CtrlpadParams *params, uint64_t low, uint64_t high, int count );
void ctrlpadSetRepeatButtons( CtrlpadParams *params, unsigned int mask );
void ctrlpadReset( CtrlpadParams *params );
void ctrlpadSetRemap( CtrlpadParams *params, CtrlpadAltBtn *altbtn, unsigned int num );
bool ctrlpadStoreRemap( CtrlpadParams *params, CtrlpadAltBtn *altbtn, unsigned int num );
void ctrlpadClearRemap( CtrlpadParams *params );
void ctrlpadUpdateData( CtrlpadParams *params );
unsigned int ctrlpadGetData( CtrlpadParams *params,SceCtrlData *pad_data, int analog_deadzone );

#ifdef __cplusplus
}
#endif

#endif

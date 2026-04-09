
#ifndef __MACRO_H__
#define __MACRO_H__

#include <pspctrl.h>
#include "psp/memsce.h"
#include "../menu.h"

#define NOTICE_DISPLAY_SEC 2

typedef bool ( *MACRO_FUNCTION )( SceCtrlData* );

typedef enum{
	MI_SETUP,
	MI_ORDER,
} MacroInterrupt;

typedef enum {
	MRM_NONE,
	MRM_TRACE,
	MRM_RECORD,
} MacroRunMode;

typedef enum {
	MA_DELAY,
	MA_BUTTONS_PRESS,
	MA_BUTTONS_RELEASE,
	MA_BUTTONS_CHANGE,
} MacroAction;

typedef struct _macro_data {
	MacroAction action;
	unsigned int data;
	struct _macro_data *next;
	struct _macro_data *prev;
} MacroData;

void macroInit( void );
void macroTerm( void );
bool macroMain( HookCaller caller, SceCtrlData *pad_data, void *argp );
bool macroMenu( SceCtrlLatch *pad_latch, SceCtrlData *pad_data, void *argp );

bool macroLoad( SceCtrlData *pad_data );
bool macroEdit( SceCtrlData *pad_data );
bool macroClear( SceCtrlData *pad_data );
bool macroRecord( SceCtrlData *pad_data );

#endif

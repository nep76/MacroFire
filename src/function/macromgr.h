/*
	macromgr.h
	
	マクロマネージャ。
	マクロの追加や削除、管理を行う。
*/

#ifndef __MACROAPI_H__
#define __MACROAPI_H__

#include <pspkernel.h>
#include "psp/memsce.h"

#define MACRO_MAX_DELAY 999999999UL

typedef enum {
	MA_DELAY = 0,
	MA_BUTTONS_PRESS,
	MA_BUTTONS_RELEASE,
	MA_BUTTONS_CHANGE,
} MacroAction;

typedef struct _macro_data {
	MacroAction action;
	u64 data;
	struct _macro_data *next;
	struct _macro_data *prev;
} MacroData;

typedef enum {
	MIP_BEFORE,
	MIP_AFTER
} MacroInsertPosition;

MacroData *macromgrNew( void );
MacroData *macromgrInsert( MacroInsertPosition pos, MacroData *macro );
void macromgrRemove( MacroData *macro );
void macromgrDestroy( void );
MacroData *macromgrGetFirst( void );
int macromgrGetCount( void );

#endif

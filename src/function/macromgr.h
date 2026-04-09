/*
	macromgr.h
	
	マクロマネージャ。
	マクロの追加や削除、管理を行う。
*/

#ifndef __MACROMGR_H__
#define __MACROMGR_H__

#include <pspkernel.h>
#include "psp/memsce.h"

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define MACRO_MAX_DELAY                               999999999UL
#define MACRO_SET_ANALOG_XY( x, y )                   ( (u64)(( (u64)( x ) << 32 ) | ( y )) )
#define MACRO_GET_ANALOG_X( x )                       ( (u32)(( x ) >> 32) )
#define MACRO_GET_ANALOG_Y( y )                       ( (u32)( y ) )
#define MACRO_ANALOG_THRESHOLD( o_x, o_y, o_r, x, y ) ( ( abs( x - o_x )^2 ) + ( abs( y - o_y )^2 ) >= ( o_r^2 ) )

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef enum {
	MA_DELAY = 0,
	MA_BUTTONS_PRESS,
	MA_BUTTONS_RELEASE,
	MA_BUTTONS_CHANGE,
	MA_ANALOG_NEUTRAL,
	MA_ANALOG_MOVE
} MacroAction;

typedef struct _macro_data {
	MacroAction action;
	uint64_t data;
	struct _macro_data *next;
	struct _macro_data *prev;
} MacroData;

typedef enum {
	MIP_BEFORE,
	MIP_AFTER
} MacroInsertPosition;

/*-----------------------------------------------
	関数
-----------------------------------------------*/
MacroData *macromgrNew     ( void );
MacroData *macromgrInsert  ( MacroInsertPosition pos, MacroData *macro );
void      macromgrRemove   ( MacroData *macro );
void      macromgrDestroy  ( void );
MacroData *macromgrGetFirst( void );
int       macromgrGetCount ( void );

#endif

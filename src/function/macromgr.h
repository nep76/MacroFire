/*
	macromgr.h
	
	マクロマネージャ。
	マクロの追加や削除、管理を行う。
*/

#ifndef MACROMGR_H
#define MACROMGR_H

#include <pspkernel.h>
#include "psp/memsce.h"
#include <limits.h>

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define MACROMGR_MAX_DELAY                999999999UL

#define MACROMGR_SET_ANALOG_XY( x, y )    ( (u64)(( (u64)( x ) << 32 ) | ( y )) )
#define MACROMGR_GET_ANALOG_X( x )        ( (u32)(( x ) >> 32) )
#define MACROMGR_GET_ANALOG_Y( y )        ( (u32)( y ) )
#define MACROMGR_ANALOG_CENTER            128
#define MACROMGR_ANALOG_NEUTRAL           MACROMGR_SET_ANALOG_XY( MACROMGR_ANALOG_CENTER, MACROMGR_ANALOG_CENTER )

#define MACROMGR_DEFAULT_PRESS_DELAY     18
#define MACROMGR_DEFAULT_RELEASE_DELAY   18
#define MACROMGR_SET_RAPIDDELAY( pd, rd ) MACROMGR_SET_ANALOG_XY( pd, rd )
#define MACROMGR_GET_RAPIDPDELAY( pd )    MACROMGR_GET_ANALOG_X( pd )
#define MACROMGR_GET_RAPIDRDELAY( rd )    MACROMGR_GET_ANALOG_Y( rd )

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef enum {
	MA_DELAY = 0,
	MA_BUTTONS_PRESS,
	MA_BUTTONS_RELEASE,
	MA_BUTTONS_CHANGE,
	MA_ANALOG_MOVE,
	MA_RAPIDFIRE_START,
	MA_RAPIDFIRE_STOP
} MacroAction;

typedef struct _macro_data {
	MacroAction action;
	uint64_t data;
	uint64_t sub;
	struct _macro_data *next;
	struct _macro_data *prev;
} MacroData;

/*-----------------------------------------------
	関数
-----------------------------------------------*/
MacroData *macromgrNew( void );
MacroData *macromgrInsertBefore( MacroData *macro );
MacroData *macromgrInsertAfter( MacroData *macro );
void macromgrRemove( MacroData *macro );
void macromgrDestroy( MacroData *macro );
void macromgrSet( MacroData *macro, MacroAction action, uint64_t data, uint64_t sub );

#endif

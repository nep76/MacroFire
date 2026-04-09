/*
	グローバルヘッダー
*/

#ifndef MACROFIRE_H
#define MACROFIRE_H

#include <pspdebug.h>
#include <stdbool.h>
#include "utils/inimgr.h"

#define MFM_TOP_MESSAGE       "MacroFire v2.1.0 In-game menu [ClassG: http://classg.sytes.net]"
#define MFM_WORKING_DIRECTORY "ms0:/seplugins"

#ifdef GLOBAL_VARIABLES_DEFINE
#define GLOBAL
#define INIT_VALUE( x ) = ( x ) /* define */
#else
#define GLOBAL extern
#define INIT_VALUE( x ) /* extern */
#endif
#define MF_ARRAY_NUM( x )  sizeof( x ) / sizeof( x[0] )

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef enum {
	MF_CALL_INTERNAL,
	MF_CALL_READ,
	MF_CALL_LATCH
} MfCallMode;

/*-----------------------------------------------
	グローバル関数 (main.c)
	
	MacroFire Engineと実際のAPIのフック状態を変更/参照する。
-----------------------------------------------*/
bool mfIsApiHooked( void );
void mfHookApi    ( void );
void mfRestoreApi ( void );
void mfEnable     ( void );
void mfDisable    ( void );
bool mfIsEnabled  ( void );
bool mfIsDisabled ( void );

/*-----------------------------------------------
	グローバル変数
-----------------------------------------------*/
GLOBAL bool         gRunning  INIT_VALUE( true );
GLOBAL bool         gMfEngine INIT_VALUE( false );
GLOBAL unsigned int gMfMenu   INIT_VALUE( 0 );
GLOBAL unsigned int gMfToggle INIT_VALUE( 0 );

#undef GLOBAL
#undef VALUE

/*-----------------------------------------------
	MFAPI
-----------------------------------------------*/
#include "menu.h"
#include "analogtune.h"
#include "rapidfire.h"

#endif

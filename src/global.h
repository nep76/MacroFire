/*
	グローバルヘッダー
*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <pspdebug.h>
#include <stdbool.h>

#define MF_MENU_TOP_MESSAGE "MacroFire v1.2.0 In-game menu [ClassG: http://classg.sytes.net]"

#ifdef GLOBAL_VARIABLES_DEFINE
#define GLOBAL
#define INIT_VALUE( x ) = ( x ) /* define */
#else
#define GLOBAL extern
#define INIT_VALUE( x ) /* extern */
#endif

#define MF_ENGINE_OFF 0
#define MF_ENGINE_ON  1
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
GLOBAL int          gMfEngine INIT_VALUE( MF_ENGINE_OFF );
GLOBAL unsigned int gMfToggle INIT_VALUE( 0 );

#undef GLOBAL
#undef VALUE

#endif

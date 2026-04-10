/*
	グローバルヘッダー
*/

#ifndef MACROFIRE_H
#define MACROFIRE_H

#include <pspdebug.h>
#include <stdbool.h>
#include "utils/inimgr.h"

#ifdef GLOBAL_VARIABLES_DEFINE
#define GLOBAL
#define INIT_VALUE( x ) = ( x ) /* define */
#else
#define GLOBAL extern
#define INIT_VALUE( x ) /* extern */
#endif
#define MF_ARRAY_NUM( x )  sizeof( x ) / sizeof( x[0] )

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define MFM_TOP_MESSAGE  "MacroFire 2.3.2 In-game menu [ClassG: http://classg.sytes.net]"
#define MFM_INI_FILENAME "ms0:/seplugins/macrofire.ini"

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef enum {
	MF_CALL_INTERNAL,
	MF_CALL_READ,
	MF_CALL_LATCH
} MfCallMode;

typedef enum {
	MF_RUNENV_VSH,
	MF_RUNENV_GAME,
	MF_RUNENV_POPS
} MfRunEnv;

/*-----------------------------------------------
	グローバル関数 (main.c)
	
	MacroFire Engineと実際のAPIのフック状態を変更/参照する。
-----------------------------------------------*/

/*
	mfIsApiHooked
	
	APIが現在フックされているかどうかを調べる。
	
	@return: bool
		true : フック済み
		false: 未フック
*/
bool mfIsApiHooked( void );

/*
	mfHookApi
	
	APIのフックを実行する。
*/
void mfHookApi    ( void );

/*
	mfRestoreApi
	
	APIのフックを解除する。
*/
void mfRestoreApi ( void );

/*
	mfEnable
	
	MacroFire EngineをONにする。
	これをONにしても、その場でAPIがフックされるわけではない。
	言うなれば、APIのフックを予約するようなもので、
	ON状態でメインルーチンに戻ったときにフックが実行される。
*/
void mfEnable     ( void );

/*
	mfDisable
	
	MacroFireEngineをOFFにする。
	mfEnable()と同様に、OFFにしてもその場でAPIのフックが解除されるわけではない。
*/
void mfDisable    ( void );

/*
	mfIsEnabled
	
	MacroFire Engineの状態を返す。
	
	@return: bool
		true : MacroFire Engine = ON
		false: MacroFire Engine = OFF
*/
bool mfIsEnabled  ( void );

/*
	mfIsDisabled
	
	MacroFire Engineの状態を返す。
	
	@return: bool
		true : MacroFire Engine = OFF
		false: MacroFire Engine = ON
*/

bool mfIsDisabled ( void );

/*
	mfRunEnv
	
	MacroFireがVSH/GAME/POPSのどこで動作しているかを返す。
	
	@return: MfRunEnv
		MF_RUNENV_VSH : VSH上で動作中。
		MF_RUNENV_GAME: GAME上で動作中。
		MF_RUNENV_POPS: POPS上で動作中。
*/
MfRunEnv mfRunEnv( void );

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

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

//#define DEBUG

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define MF_VERSION      "2.4.0"
#define MF_INI_FILENAME "ms0:/seplugins/macrofire.ini"

#define MF_UNUSED_BUTTONS ( PSP_CTRL_WLAN_UP | PSP_CTRL_REMOTE | PSP_CTRL_DISC | PSP_CTRL_MS )
#define MF_KERNEL_BUTTONS ( PSP_CTRL_NOTE | PSP_CTRL_SCREEN | PSP_CTRL_VOLUP | PSP_CTRL_VOLDOWN | MFM_UNUSED_BUTTONS )

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

/*
	mfButtonsMask
	
	ボタンデータから指定したボタン以外を取り除く。
	
	@param: unsigned int buttons
		ボタンデータ。
	
	@param: unsigned int umask
		取り除くボタンデータ。
	
	@return: unsigned int
		処理されたボタン。
*/
unsigned int mfButtonsMask( unsigned int buttons, unsigned int mask );

/*
	mfButtonsMask
	
	ボタンデータから指定したボタンを取り除く。
	
	@param: unsigned int buttons
		ボタンデータ。
	
	@param: unsigned int umask
		取り除くボタンデータ。
	
	@return: unsigned int
		処理されたボタン。
*/
unsigned int mfButtonsUmask( unsigned int buttons, unsigned int umask );

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

/*
	グローバルヘッダー
*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <pspdebug.h>
#include <stdbool.h>

#define MF_MENU_TOP_MESSAGE "MacroFire v1.1.0 In-game menu [ClassG: http://classg.sytes.net]"

#ifdef GLOBAL_VARIABLES_DEFINE
#define GLOBAL
#define INIT_VALUE( x ) = ( x ) /* define */
#else
#define GLOBAL extern
#define INIT_VALUE( x ) /* extern */
#endif

#define MFENGINE_OFF 0
#define MFENGINE_ON  1

#define ARRAY_NUM( x )  sizeof( x ) / sizeof( x[0] )

typedef int ( *SCE_CTRL_DATA_FUNC )( SceCtrlData*, int );
typedef int ( *SCE_CTRL_LATCH_FUNC )( SceCtrlLatch* );

/* ファンクションの呼び出しモード */
typedef enum {
	MF_CALL_READ,
	MF_CALL_LATCH
} MfCallMode;

/* グローバル関数 */
bool mfIsApiHooked( void );
void mfHookApi( void );
void mfRestoreApi( void );
void mfEnable( void );
void mfDisable( void );
bool mfIsEnabled( void );
bool mfIsDisabled( void );

/* グローバル変数 */
GLOBAL bool gRunning INIT_VALUE( true );

/* グローバル設定記録変数 */
GLOBAL int gMfEngine INIT_VALUE( 0 );
GLOBAL unsigned int gMfToggle INIT_VALUE( 0 );

#undef GLOBAL
#undef VALUE

#endif

/*
	グローバルヘッダー
*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <pspdebug.h>
#include <stdbool.h>

#define MF_MENU_TOP_MESSAGE "MacroFire vBETA In-game menu [ClassG: http://classg.sytes.net]"

#ifdef GLOBAL_VARIABLES_DEFINE
#define GLOBAL
#define INIT_VALUE( x ) = ( x ) /* define */
#else
#define GLOBAL extern
#define INIT_VALUE( x ) /* extern */
#endif
	
#define ARRAY_NUM( x )  sizeof( x ) / sizeof( x[0] )

typedef int ( *SCE_CTRL_DATA_FUNC )( SceCtrlData*, int );
typedef int ( *SCE_CTRL_LATCH_FUNC )( SceCtrlLatch* );

/* グローバル変数 */
GLOBAL bool gRunning  INIT_VALUE( true );

/* グローバル設定記録変数 */
GLOBAL int gMfEngine INIT_VALUE( 0 );



#undef GLOBAL
#undef VALUE

#endif

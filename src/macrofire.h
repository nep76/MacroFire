/*==========================================================
	
	MacroFire v3
	
	MacroFire API。
	
==========================================================*/
#ifndef MACROFIRE_H
#define MACROFIRE_H

#ifdef PSP_USE_KERNEL_LIBC
#include "psp/sysclib/sysclib.h"
#endif

#include "pspapi.h"
#include "mftypes.h"
#include "mftab.h"

/*==========================================================
	定数
==========================================================*/
#define MF_TITLE        "MacroFire %s In-game menu"
#define MF_VERSION      "3.0.5"
#define MF_AUTHOR       "ClassG (http://classg.sytes.net)"

#define MF_INI_PATH_DEFAULT    "ms0:/seplugins/"
#define MF_INI_FILENAME        "macrofire.ini"
#define MF_INI_SECTION_DEFAULT "Default"
#define MF_INI_SECTION_VSH     "VSH"
#define MF_INI_SECTION_POPS    "POPS"
#define MF_INI_SECTION_GAME    "GAME"
#define MF_INI_LOAD_FAILED     "Built-in"

#define MF_COLOR_TRANSPARENT GB_TRANSPARENT
#define MF_COLOR_TEXT_FG     0xffffffff
#define MF_COLOR_TEXT_BG     MF_COLOR_TRANSPARENT
#define MF_COLOR_TEXT_FC     0xff0000ff
#define MF_COLOR_TEXT_INACT  0x44ffffff
#define MF_COLOR_TEXT_LABEL  0x88ccffff
#define MF_COLOR_BG          0xbb000000
#define MF_COLOR_TITLE_BAR   0x44aaaaaa
#define MF_COLOR_TITLE       0xffffffff
#define MF_COLOR_INFO_BAR    0x44aaaaaa
#define MF_COLOR_INFO        0xffffffff
#define MF_COLOR_EX1         0xffff0000
#define MF_COLOR_EX2         0xffffff00
#define MF_COLOR_EX3         0xffff00ff
#define MF_COLOR_EX4         0xff00ffff

/*==========================================================
	マクロ
==========================================================*/
/*-----------------------------------------------
	デバッグ用。
	DEBUGが真の場合、dbgprint/dbgprintfが有効になる。
	
	MakefileのCFLAGSに -DDEBUG=X
	
	X
	0: デバッグ無効
	1: デバッグメッセージを標準出力へ
	   ファイルへ出力する場合は明示的にmfDebugPrintf()を使用するが、
	   DEBUG=0にしても自動で削除されないため一時的な使用に限る。
	2: デバッグメッセージをms0:/mfdebug.txtへ
-----------------------------------------------*/
#if DEBUG >= 1
#define DEBUG_ENABLED
#endif

#ifdef MF_FIRST_INCLUDE
#define GLOBAL
#define INIT( x ) = x
#ifdef DEBUG_ENABLED
int mfDebugPrintf( const char *format, ... )
{
	SceUID fd;
	va_list ap;
	char buf[255];
	unsigned short len = 0;
	
	va_start( ap, format );
	vsnprintf( buf, 255, format, ap );
	va_end( ap );
	
	fd = sceIoOpen( "ms0:/mfdebug.txt", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
	if( fd > 0 ){
		len = strlen( buf );
		sceIoWrite( fd, buf, len );
		sceIoClose( fd );
	}
	
	return len;
}
#endif
#else
#define GLOBAL extern
#define INIT( x )
#ifdef DEBUG_ENABLED
int mfDebugPrintf( const char *format, ... );
#endif
#endif

#if DEBUG >= 2
#define DEBUG_PRINTF mfDebugPrintf
#elif DEBUG >= 1
#define DEBUG_PRINTF printf
#endif

#ifdef DEBUG_ENABLED
#define dbgprintf( fmt, ... ) \
	DEBUG_PRINTF( "(%s():%s:%d): ", __func__, __FILE__, __LINE__ );\
	DEBUG_PRINTF( fmt, __VA_ARGS__ );\
	DEBUG_PRINTF( "\n" );
#define dbgprint( str ) \
	DEBUG_PRINTF( "(%s():%s:%d): ", __func__, __FILE__, __LINE__ );\
	DEBUG_PRINTF( str );\
	DEBUG_PRINTF( "\n" );
#else
#define dbgprintf( fmt, ... )
#define dbgprint( str )
#endif

/*==========================================================
	グローバル変数
==========================================================*/
GLOBAL bool gRunning INIT( true );

/*=========================================================
	MacroFire API
=========================================================*/
void mfHook( void );
void mfUnhook( void );
void mfEnable( void );
void mfDisable( void );
bool mfIsEnabled( void );
void mfSetMenuButtons( PadutilButtons buttons );
void mfSetToggleButtons( PadutilButtons buttons );
PadutilButtons mfGetMenuButtons( void );
PadutilButtons mfGetToggleButtons( void );
bool mfConvertButtonReady( void );
void mfConvertButtonFinish( void );
char *mfConvertButtonC2N( PadutilButtons buttons, char *buf, size_t len );
PadutilButtons mfConvertButtonN2C( char *buttons );
const char *mfGetGameId( void );
const char *mfGetIniTargetSection( void );
const char *mfGetIniSection( void );
unsigned int mfGetWorld( void );
bool mfIsRunningApp( MfAppId app );
int mfOverlayMessageStart( void );
void mfOverlayMessageExit( void );
bool mfOverlayMessageIsRunning( void );
bool mfOverlayMessagePrintf( const char *format, ... );
bool mfHookIncomplete( void );
HeapUID mfHeapCreate( unsigned int count, size_t size );
#define mfHeapAlloc   heapAlloc
#define mfHeapCalloc  heapCalloc
#define mfHeapFree    heapFree
#define mfHeapDestroy heapDestroy

#ifndef MFEXCLUDE_DIALOG
#include "mfdialog.h"
#endif

#ifndef MFEXCLUDE_MENU
#include "mfmenu.h"
#endif

#ifndef MFEXCLUDE_CTRL
#include "mfctrl.h"
#endif

#ifndef MFEXCLUDE_ANALOGSTICK
#include "mfanalogstick.h"
#endif

#ifndef MFEXCLUDE_RAPIDFIRE
#include "mfrapidfire.h"
#endif

#endif

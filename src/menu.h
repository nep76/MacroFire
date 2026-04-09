/*
	menu.h
*/

#ifndef __MFMENU_H__
#define __MFMENU_H__

#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdbool.h>
#include <string.h>

#include "global.h"
#include "sstring.h"
#include "psp/blit.h"
#include "psp/memsce.h"

#define MENU_FGCOLOR 0xffffffff
#define MENU_BGCOLOR 0xff000000
#define MENU_FCCOLOR 0xff0000ff

#define MAX_NUMBER_OF_THREADS 64
#define MENUITEM_LENGTH 64

/*
	sceDisplayEnable()でLCDを点灯できるようだ?
	ただ、Stubsには追加されているがプロトタイプが無い模様。
*/
void sceDisplayEnable( void );

typedef enum {
	CALL_PEEK_BUFFER_POSITIVE,
	CALL_PEEK_BUFFER_NEGATIVE,
	CALL_READ_BUFFER_POSITIVE,
	CALL_READ_BUFFER_NEGATIVE,
	CALL_PEEK_LATCH,
	CALL_READ_LATCH
} HookCaller;

typedef struct _mf_menu_chain {
	char *mid;
	struct _mf_menu_chain *prev;
} MfMenuChain;

typedef enum {
	MR_BACK,
	MR_ENTER,
	MR_CONTINUE
} MfMenuReturnCode;

typedef enum {
	MM_NEXT,
	MM_PREV,
} MfMenuMoveFocusDir;

typedef enum {
	MT_ANCHOR,
	MT_OPTION,
	MT_BORDER,
	MT_CALL,
} MfMenuItemType;

typedef struct {
	MfMenuItemType type;
	int  *selected;
	char *label;
	char *value[10];
} MfMenuItem;

typedef struct {
	char mid[32];
	MfMenuItem *mi;
	int micount;
} MfMenuIndex;

void mfMenuSetup(
	SCE_CTRL_LATCH_FUNC ctrlReadLatch,
	SCE_CTRL_DATA_FUNC ctrlPeekBufferPositive,
	SceCtrlLatch *pad_latch,
	SceCtrlData *pad_data
);

bool mfMenuInit( void );
void mfMenuTerm( void );
void mfMenu( void );
void mfThreadsStatChange( bool stat, SceUID thlist[], int thnum );


/* MenuAPI */
/*
	ユーザの入力なしに、ソフト側からメニューの終了を予約させる。
	これをセットしたあと、ファンクションがメインメニューに戻った時に終了。
*/
void mfMenuQuit( void );

/*
	単純な縦並びメニューを描画。
	MfMenuItem配列を渡す。
	
	選択したメニューアイテムがオプションの場合は、MfMenuItemのselectedメンバのポインタに
	オプションの通し番号がセットされる。
	
	メニューがアンカーの場合は、第5引数のselected変数に項目位置の番号と
	返り値としてMR_ENTERが返る。
	
	×ボタンが押されるとMR_BACKが返る。
*/
MfMenuReturnCode mfMenuVertical( int x, int y, int w, MfMenuItem mi[], size_t items_num, int *selected );

/* スクリーンをクリア */
void mfClearColor( u32 color );

/* メニュー実行の一時停止を予約 */
void mfWaitScreenReload( int sec );

/* VSYNC時に画面をクリアすることを予約 */
void mfClearScreenWhenVsync( void );

/* メニューの中断を有効にする */
void mfMenuEnableInterrupt( void );

/* メニューの中断を無効にする */
void mfMenuDisableInterrupt( void );

#endif

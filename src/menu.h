/*
	menu.h
*/

#ifndef MFMENU_H
#define MFMENU_H

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspsysmem_kernel.h>
#include <pspsuspend.h>

#include "psp/memsce.h"
#include "psp/gb.h"
#include "psp/cmndlg.h"
#include "psp/ctrlpad.h"
#include "utils/strutil.h"

#define MFM_TOP_MESSAGE "MacroFire %s In-game menu [ClassG: http://classg.sytes.net]"

#define MFM_DISPLAY_MICROSEC_INFO  1000000
#define MFM_DISPLAY_MICROSEC_ERROR 3000000

#define MFM_TRANSPARENT      GB_TRANSPARENT
#define MFM_BG_COLOR         0xbb000000
#define MFM_TITLE_BAR_COLOR  0x44aaaaaa
#define MFM_TITLE_TEXT_COLOR 0xffffffff
#define MFM_INFO_BAR_COLOR   0x44aaaaaa
#define MFM_INFO_TEXT_COLOR  0xffffffff
#define MFM_TEXT_FGCOLOR     0xffffffff
#define MFM_TEXT_BGCOLOR     MFM_TRANSPARENT
#define MFM_TEXT_FCCOLOR     0xff0000ff

#define MFM_OUT_OF_MEM_WARN_SEC 6
#define MFM_MAIN_MENU_COUNT 6

#define MFM_MAX_NUMBER_OF_THREADS 64
#define MFM_ITEM_LENGTH 64

#define MFM_GET_CB_ARG_BY_PTR( p, n ) ( ((MfMenuItemValue *)( p ))[( n )].pointer )
#define MFM_GET_CB_ARG_BY_INT( p, n ) ( ((MfMenuItemValue *)( p ))[( n )].integer )
#define MFM_GET_CB_ARG_BY_STR( p, n ) ( ((MfMenuItemValue *)( p ))[( n )].string )

#define MFM_ALL_AVAILABLE_BUTTONS ( \
	PSP_CTRL_CIRCLE   | PSP_CTRL_CROSS    | PSP_CTRL_SQUARE | PSP_CTRL_TRIANGLE | \
	PSP_CTRL_UP       | PSP_CTRL_RIGHT    | PSP_CTRL_DOWN   | PSP_CTRL_LEFT     | \
	PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_SELECT | PSP_CTRL_START    | \
	PSP_CTRL_NOTE     | PSP_CTRL_SCREEN   | PSP_CTRL_VOLUP  | PSP_CTRL_VOLDOWN  | \
	CTRLPAD_CTRL_ANALOG_UP   | CTRLPAD_CTRL_ANALOG_RIGHT | \
	CTRLPAD_CTRL_ANALOG_DOWN | CTRLPAD_CTRL_ANALOG_LEFT \
)

/*-----------------------------------------------
	プロトタイプのないAPI
-----------------------------------------------*/
void sceDisplayEnable( void );

/*
	背景のような大きなメモリブロックの転送にはDMAコントローラを使用。
	細切れのデータに対しては逆に遅くなるとの情報があったので背景転送にのみ使う。
	本当に遅いのかどうかは調べていない。
*/

// 同期版？
// とりあえずmemcpy()と単純置換しても問題は起きず高速化した。
int  sceDmacMemcpy( void *dest, const void *src, SceSize size );

// 連続で行うとなにか変。
// 多分、非同期版。
//int  sceDmacTryMemcpy( void *dest, const void *src, SceSize size );

/*
	カーネルメモリのロック/アンロック
*/
//int scePowerVolatileMemLock( int unk, void **ptr, int *size );    /* 同期 */
int scePowerVolatileMemTryLock( int unk, void **ptr, int *size ); /* 非同期 */
int scePowerVolatileMemUnlock( int unk );

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
enum mf_thread_chstat {
	MFM_THREADS_SUSPEND,
	MFM_THREADS_RESUME
};


struct mf_fbstat {
	int mode;
	int width;
	int height;
	void *frameBuffer;
	int bufferWidth;
	enum PspDisplayPixelFormats pixelFormat;
};

struct mf_buffers {
	struct {
		void *vram;
		void *origAddr;
	} backup;
	
	struct {
		void *display;
		void *draw;
		void *clearImage;
		unsigned int size;
	} frame;
	
	bool useVolatileMem;
};

typedef enum {
	MO_DISPLAY_ONLY = 0x00000001,
} MfMenuOptions;

typedef enum {
	MR_CONTINUE,
	MR_BACK,
	MR_ENTER,
} MfMenuRc;

typedef enum {
	MT_NULL = 0,
	MT_ANCHOR,
	MT_CALLBACK,
	MT_BOOL,
	MT_OPTION,
	MT_GET_NUMBERS,
	MT_GET_BUTTONS,
} MfMenuItemType;

typedef union {
	void *pointer;
	char *string;
	int  integer;
} MfMenuItemValue;

typedef struct {
	void *func;
	MfMenuItemValue *arg;
} MfMenuCallback;

typedef enum {
	MS_NONE = 0,
	MS_QUERY_LABEL,
	MS_QUERY_USAGE,
	MS_FOCUS,
	MS_BLUR,
} MfMenuCtrlSignal;

typedef MfMenuRc ( *MfMenuProc )( MfMenuCtrlSignal, SceCtrlData*, void*, MfMenuItemValue*, const void* );

typedef struct {
	char            *label;
	MfMenuProc      proc;
	void            *var;
	MfMenuItemValue value[10];
} MfMenuItem;

/*-----------------------------------------------
	関数
-----------------------------------------------*/
bool mfMenuInit( void );
void mfMenu( void );

/*-----------------------------------------------
	メニュー用API
-----------------------------------------------*/
/*
	メニュー終了フラグをセットする。
*/
void mfMenuQuit( void );

/*
	STARTボタン、HOMEボタンによるメニューの中断を無効にする。
*/
void mfMenuDisableInterrupt( void );

/*
	STARTボタン、HOMEボタンによるメニューの中断を有効に戻す。
*/
void mfMenuEnableInterrupt( void );

/*
	指定された時間だけ、画面リロード時に停止する。
	
	@param: uint64_t micro_sec
		停止するマイクロ秒。
*/
void mfMenuWait( uint64_t micro_sec );

/*
	キーリピート時間をリセットする。
*/
void mfMenuKeyRepeatReset( void );

/*
	未実装。
*/
MfMenuRc mfMenuMultiDraw( int x, int y, MfMenuItem **menu, int row, int col, int *select_row, int *select_col, MfMenuOptions options );

/*
	単一カラムのメニューを表示する。
	
	@param: int x
		表示X位置。
	
	@param: int y
		表示Y位置。
	
	@param: MfMenuItem menu[]
		MfMenuItem構造体配列。
	
	@param: size_t num
		上記MfMenuItem構造体配列の要素数。
	
	@param: int *select
		選択項目のインデックス番号を保存するポインタ。
	
	@param: MfMenuOptions option
		表示オプション。
*/
MfMenuRc mfMenuUniDraw( int x, int y, MfMenuItem menu[], size_t num, int *select, MfMenuOptions options );

bool mfMenuGetNumberIsReady( void );
MfMenuRc mfMenuGetNumberInit( const char *title, const char *unit, long *number, int digits );
MfMenuRc mfMenuGetNumber( void );

bool mfMenuGetButtonsIsReady( void );
MfMenuRc mfMenuGetButtonsInit( const char *title, unsigned int *buttons, unsigned int avail );
MfMenuRc mfMenuGetButtons( void );

bool mfMenuGetFilenameIsReady( void );
bool mfMenuGetFilenameInit( const char *title, unsigned int flags, const char *initpath, size_t namelen, size_t pathlen );
bool mfMenuGetFilename( void );
bool mfMenuGetFilenameResult( char *path, size_t len, int *split );

void mfMenuLabel( const char *format, ... );
void mfMenuUsage( const char *format, ... );

/* デフォルトプロシージャ */
MfMenuRc mfMenuDefAnchorProc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg );
MfMenuRc mfMenuDefCallbackProc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg );
MfMenuRc mfMenuDefBoolProc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg );
MfMenuRc mfMenuDefOptionProc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg );
MfMenuRc mfMenuDefGetNumberProc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg );
MfMenuRc mfMenuDefGetButtonsProc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg );

#endif

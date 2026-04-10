/*=========================================================

	mfcommontypes.h

	MacroFire 共有型宣言。

=========================================================*/
#ifndef MFCOMMONTYPES_H
#define MFCOMMONTYPES_H

#include <pspkernel.h>
#include <psphprm.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include "util/strutil.h"
#include "file/inimgr.h"
#include "pad/padctrl.h"
#include "pad/padutil.h"
#include "graphic/pb.h"

/*=========================================================
	マクロ
=========================================================*/
#define MF_SIZEOF_ARRAY( x ) sizeof( x ) / sizeof( x[0] )
#define MF_PATH_MAX 255

#define MF_TARGET_BUTTONS ( \
	PSP_CTRL_CIRCLE   | PSP_CTRL_CROSS    | PSP_CTRL_SQUARE | PSP_CTRL_TRIANGLE | \
	PSP_CTRL_UP       | PSP_CTRL_RIGHT    | PSP_CTRL_DOWN   | PSP_CTRL_LEFT     | \
	PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_SELECT | PSP_CTRL_START    | \
	PADUTIL_CTRL_ANALOG_UP   | PADUTIL_CTRL_ANALOG_RIGHT | \
	PADUTIL_CTRL_ANALOG_DOWN | PADUTIL_CTRL_ANALOG_LEFT  | \
	PSP_CTRL_HOME \
)

#define MF_HOTKEY_BUTTONS (\
	padutilSetPad( MF_TARGET_BUTTONS | ( PSP_CTRL_NOTE | PSP_CTRL_SCREEN | PSP_CTRL_VOLUP | PSP_CTRL_VOLDOWN ) ) |\
	padutilSetHprm( PADUTIL_HPRM_NORMAL_KEYS ) \
)

/*=========================================================
	型宣言
=========================================================*/
/*-----------------------------------------------
	実行環境を表す数値。
-----------------------------------------------*/
typedef enum {
	MF_WORLD_GAME = 0x00000001,
	MF_WORLD_POPS = 0x00000002,
	MF_WORLD_VSH  = 0x00000004,
} MfWorldId;

/*-----------------------------------------------
	実行中のアプリケーション識別番号。
	
	この値をmfFindApp()に渡すと、そのアプリケーションが起動中か調べる。
	今のところ、Netfront(ウェブブラウザ)しか判定していない。
-----------------------------------------------*/
typedef enum {
	MF_APP_WEBBROWSER = 0x00000001,
	/*
	MF_APP_MUSICPLAYER = 0x00000002,
	MF_APP_VIDEO       = 0x00000004,
	MF_APP_ONESEG      = 0x00000008,
	*/
} MfAppId;

/*-----------------------------------------------
	CPUの実行モード。
-----------------------------------------------*/
typedef enum {
	MF_CALLER_KERNEL = 0,
	MF_CALLER_USER
} MfCallerMode;

/*-----------------------------------------------
	パッドデータの取得方法
-----------------------------------------------*/
typedef enum {
	MF_INTERNAL = 0,
	MF_KEEP,
	MF_UPDATE,
} MfHookAction;

/*-----------------------------------------------
	HPRMキー型
-----------------------------------------------*/
typedef u32 MfHprmKey;

/*-----------------------------------------------
	メッセージ
-----------------------------------------------*/
typedef unsigned int MfMessage;
typedef enum {
	MF_MS_INIT = 0,
	MF_MS_INI_LOAD,
	MF_MS_INI_CREATE,
	MF_MS_TERM,
	MF_MS_HOOK,
	MF_MS_TOGGLE,
	MF_MS_MENU,
} MfMainMessage;

typedef void ( *MfFuncInit   )( void );
typedef void ( *MfFuncIni    )( IniUID, char*, size_t );
typedef void ( *MfFuncTerm   )( void );
typedef void ( *MfFuncHook   )( MfHookAction, SceCtrlData*, MfHprmKey* );
typedef void ( *MfFuncToggle )( bool );
typedef void ( *MfFuncMenu   )( MfMessage );

/*-----------------------------------------------
	プロシージャ
-----------------------------------------------*/
typedef void *( *MfProc )( MfMainMessage message );

#endif

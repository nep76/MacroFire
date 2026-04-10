/*
	function table
	
	MacroFireがフック時に呼び出す関数と、メニュー表示時に呼び出す関数のテーブル。
*/

#ifndef MFFUNCTIONTABLE_H
#define MFFUNCTIONTABLE_H

#ifdef MFTABLE_DEFINE
#define EXPORT
#else
#define EXPORT extern
#endif

#include "macrofire.h"
#include "utils/inimgr.h"

/*-----------------------------------------------
	機能を実装したソースのヘッダ
-----------------------------------------------*/
#include "function/remap.h"
#include "function/rapidfire.h"
#include "function/macro.h"

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef void     ( *MfFuncInit )( void );
typedef void     ( *MfFuncIni  )( IniUID, char*, size_t );
typedef void     ( *MfFuncTerm )( void );
typedef void     ( *MfFuncHook )( MfCallMode, SceCtrlData*, void* );
typedef MfMenuRc ( *MfFuncMenu )( SceCtrlData*, void* );
typedef void     ( *MfFuncIntr )( const bool mfengine );

typedef struct {
	MfFuncHook func;
	MfFuncIntr intr;
	void *arg;
} MfHookEntry;

typedef struct {
	MfFuncMenu func;
	MfFuncTerm quit;
	void *arg;
} MfMenuEntry;

typedef struct {
	char         *name;
	MfFuncInit   initFunc;
	MfFuncTerm   termFunc;
	MfFuncIni    ciFunc, liFunc;
	MfHookEntry  hook;
	MfMenuEntry  menu;
} MfEntry;

/*-----------------------------------------------
	MacroFire ファンクションエントリ
-----------------------------------------------*/
EXPORT MfEntry mftable[]
#ifdef MFTABLE_DEFINE
= {
	{
		"Remap settings",
		NULL, remapTerm,
 		remapCreateIni, remapLoadIni,
		{ remapMain, NULL },
		{ remapMenu, NULL }
	},
	
	{
		"Rapidfire settings",
		NULL, NULL,
 		rapidfireCreateIni, rapidfireLoadIni,
		{ NULL, NULL },
		{ rapidfireMenu, rapidfireApply }
	},
	
	{
		"Macro settings",
		macroInit, macroTerm,
		macroCreateIni, macroLoadIni,
		{ macroMain, macroIntr },
		{ macroMenu, NULL }
	},
}
#endif
;
EXPORT int mftableEntry
#ifdef MFTABLE_DEFINE
= sizeof( mftable ) / sizeof( MfEntry )
#endif
;

#undef EXPORT

#endif

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

#include "utils/inimgr.h"

/*-----------------------------------------------
	機能を実装したソースのヘッダ
-----------------------------------------------*/
#include "function/analogtune.h"
#include "function/rapidfire.h"
#include "function/macro.h"

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef void     ( *MfFuncInit )( void );
typedef void     ( *MfFuncIni  )( IniUID );
typedef void     ( *MfFuncTerm )( void );
typedef void     ( *MfFuncHook )( MfCallMode, SceCtrlData*, void* );
typedef MfMenuRc ( *MfFuncMenu )( SceCtrlData*, void* );
typedef void     ( *MfFuncIntr )( const bool mfengine );

typedef struct {
	MfFuncHook func;
	void *arg;
} MfHookEntry;

typedef struct {
	MfFuncMenu func;
	void *arg;
} MfMenuEntry;

typedef struct {
	char         *name;
	MfFuncInit   initFunc;
	MfFuncTerm   termFunc;
	MfFuncIntr   intrFunc;
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
	/*
		機能を定義するテーブル
		
		割込関数は、MacroFire Engineが切り替えられた次のループで呼ばれる
	*/
	{
		"Analog stick sensitivity settings",
		NULL, NULL, NULL,
		analogtuneCreateIni, analogtuneLoadIni,
		{ analogtuneMain, NULL },
		{ analogtuneMenu, NULL }
	},
	
	{
		"Rapidfire settings",
		rapidfireInit, NULL, NULL,
 		NULL, NULL,
		{ rapidfireMain, NULL },
		{ rapidfireMenu, NULL }
	},
	
	{
		"Macro settings",
		macroInit, NULL, macroIntr,
		NULL, NULL,
		{ macroMain, NULL },
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

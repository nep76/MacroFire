/*
	function table
	
	MacroFireがフック時に呼び出す関数と、メニュー表示時に呼び出す関数のテーブル。
*/

#ifndef __FUNCTIONTABLE_H__
#define __FUNCTIONTABLE_H__

#ifdef MFTABLE_DEFINE
#define EXPORT
#else
#define EXPORT extern
#endif

/* 機能を実装したソース */
#include "function/rapidfire.h"
#include "function/macro.h"

typedef void ( *MfFuncInitTerm )( void );
typedef void ( *MfFuncHook )( MfCallMode, SceCtrlData*, void* );
typedef MfMenuReturnCode ( *MfFuncMenu )( SceCtrlLatch*, SceCtrlData*, void* );
typedef void ( *MfFuncIntr )( const int mfengine );

typedef struct {
	MfFuncHook func;
	void *arg;
} HookEntry;

typedef struct {
	char *label;
	MfFuncMenu mainFunc;
	void *arg;
} MenuEntry;

typedef struct {
	MfFuncInitTerm initFunc;
	MfFuncInitTerm termFunc;
	MfFuncIntr intrFunc;
	HookEntry hook;
	MenuEntry menu;
} MfEntry;

EXPORT MfEntry mftable[]
#ifdef MFTABLE_DEFINE
= {
	/*
		機能を定義するテーブル
		{ 初期化関数, 終了関数, 割込関数, { フック関数, 引数 }, { メニュー文字列, メニュー関数, 引数 } }
		
		割込関数は、MacroFire Engineが切り替えられた次のループで呼ばれる
	*/
	{ NULL,       NULL, NULL,      { rapidfireMain, NULL }, { "Rapidfire settings", rapidfireMenu, NULL } },
	{ macroInit,  NULL, macroIntr, { macroMain,     NULL }, { "Macro settings",     macroMenu,     NULL } },
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

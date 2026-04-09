/*
	function table
	
	MacroFireがフック時に呼び出す関数と、メニュー表示時に呼び出す関数のテーブル。
*/

#ifndef __FUNCTIONTABLE_H__
#define __FUNCTIONTABLE_H__

#include "function/rapidfire.h"
#include "function/macro.h"

typedef void ( *MfFuncInitTerm )( void );
typedef bool ( *MfFuncMenu )( SceCtrlLatch*, SceCtrlData*, void* );
typedef bool ( *MfFuncHook )( HookCaller, SceCtrlData*, void* );

typedef struct {
	MfFuncHook func;
	void *arg;
} HookEntry;

typedef struct {
	char *label;
	MfFuncMenu func;
	void *arg;
} MenuEntry;

#ifdef MFTABLE_DEFINE
int mftableInitEntry = 1;
MfFuncInitTerm mftableInit[] = {
	macroInit,
};

int mftableTermEntry = 1;
MfFuncInitTerm mftableTerm[] = {
	macroTerm,
};

int mftableHookEntry = 2;
HookEntry mftableHook[] = {
	{ rapidfireMain, 0 },
	{ macroMain,     0 },
};

int mftableMenuEntry = 2;
MenuEntry mftableMenu[] = {
	{ "Rapidfire settings", rapidfireMenu, 0 },
	{ "Macro settings", macroMenu, 0 },
};
#else
extern int mftableInitEntry;
extern MfFuncInitTerm mftableInit[];
extern int mftableTermEntry;
extern MfFuncInitTerm mftableTerm[];
extern int mftableHookEntry;
extern HookEntry mftableHook[];
extern int mftableMenuEntry;
extern MenuEntry mftableMenu[];
#endif


#endif

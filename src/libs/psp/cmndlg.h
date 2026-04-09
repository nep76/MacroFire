/*
	cmndlg.h
	
	ダイアログ表示。
	表示すると、終了まで呼び出し元に制御が戻らない。
*/

#ifndef __CMNDLG_H__
#define __CMNDLG_H__

#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdbool.h>
#include "psp/cmndlg/errors.h"

#ifndef __CMNDLG_FUNCTION_EXPORT__

#include "psp/cmndlg/getdigits.h"
#include "psp/cmndlg/getbuttons.h"
	
#endif

#ifdef __cplusplus
extern "C" {
#endif

void cmndlgInit( void );
void cmndlgShutdown( void );
void cmndlgLock( void );
void cmndlgUnlock( void );

u32 cmndlgGetFgColor( void );
u32 cmndlgGetBgColor( void );
u32 cmndlgGetFcColor( void );
void cmndlgSetFgColor( u32 color );
void cmndlgSetBgColor( u32 color );
void cmndlgSetFcColor( u32 color );
bool cmndlgErrorCodeIsSce( int errcode );


#ifdef __cplusplus
}
#endif

#endif

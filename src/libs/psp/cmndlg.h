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

#include "psp/cmndlg/msg.h"
#include "psp/cmndlg/getdigits.h"
#include "psp/cmndlg/getbuttons.h"
#include "psp/cmndlg/getfilename.h"
	
#endif

#define CMNDLG_DEFAULT_FGCOLOR 0xffffffff
#define CMNDLG_DEFAULT_BGCOLOR 0xff000000
#define CMNDLG_DEFAULT_FCCOLOR 0xff0000ff

#ifdef __cplusplus
extern "C" {
#endif

void cmndlgInit( void );
void cmndlgShutdown( void );
void cmndlgLock( void );
void cmndlgUnlock( void );

bool cmndlgErrorCodeIsSce( int errcode );


#ifdef __cplusplus
}
#endif

#endif

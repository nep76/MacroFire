/*
	cmndlg.h
	
	ダイアログ表示。
*/

#ifndef CMNDLG_H
#define CMNDLG_H

#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdbool.h>
#include "psp/ctrlpad.h"
#include "cgerrs.h"

#define CMNDLG_DEFAULT_FGCOLOR 0xffffffff
#define CMNDLG_DEFAULT_BGCOLOR 0x66000000
#define CMNDLG_DEFAULT_FCCOLOR 0xff0000ff

#ifdef __cplusplus
extern "C" {
#endif

struct cmndlg_ctrl_remap{
	CtrlpadAltBtn *list;
	unsigned int  num;
};

typedef enum {
	CMNDLG_NONE = 0,
	CMNDLG_INIT,
	CMNDLG_VISIBLE,
	CMNDLG_SHUTDOWN,
} CmndlgState;

typedef enum {
	CMNDLG_UNKNOWN = 0,
	CMNDLG_CANCEL,
	CMNDLG_ACCEPT
} CmndlgRc;

typedef struct {
	int x, y, w, h;
	uint32_t fgTextColor, fcTextColor, bgTextColor, bgColor, borderColor;
} CmndlgUI;

typedef struct {
	CmndlgState state;
	void *tempBuffer;
} CmndlgBaseParams;

#ifndef CMNDLG_FUNCTION_EXPORT

#include "psp/cmndlg/message.h"
#include "psp/cmndlg/getnumbers.h"
#include "psp/cmndlg/getbuttons.h"
#include "psp/cmndlg/getfilename.h"

#endif

void cmndlgInit( void );
void cmndlgShutdown( void );
void cmndlgLock( void );
void cmndlgUnlock( void );
void cmndlgSetAlternativeButtons( CtrlpadAltBtn *altbtn, unsigned int num );
CtrlpadAltBtn *cmndlgGetAlternativeButtonsList( void );
unsigned int cmndlgGetAlternativeButtonsListCount( void );
void cmndlgClearAlternativeButtons( void );

bool cmndlgErrorCodeIsSce( int errcode );


#ifdef __cplusplus
}
#endif

#endif

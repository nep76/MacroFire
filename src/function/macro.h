
#ifndef __MACRO_H__
#define __MACRO_H__

#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include "psp/memsce.h"
#include "psp/cmndlg.h"
#include "psp/fileh.h"
#include "../menu.h"

#include "macromgr.h"
#include "macroeditor.h"

#define MACRO_MENU_OFFSET 4
#define MACRO_MENU_BASECONF 2
#define MACRO_NOTICE_DISPLAY_SEC 1.5
#define MACRO_ERROR_DISPLAY_SEC  3

/* マクロアクション識別子 */
#define MACRO_FILE_RECORD_SEPARATOR '\t'
#define MACRO_ACTION_DELAY      "Delay"
#define MACRO_ACTION_BTNPRESS   "ButtonsPress"
#define MACRO_ACTION_BTNRELEASE "ButtonsRelease"
#define MACRO_ACTION_BTNCHANGE  "ButtonsChange"
#define MACRO_ACTION_ALNEUTRAL  "AnalogNeutral"
#define MACRO_ACTION_ALMOVE     "AnalogMove"

typedef MfMenuReturnCode ( *MacroFunction )( SceCtrlLatch*, SceCtrlData* );

typedef enum {
	MRM_NONE = 0,
	MRM_TRACE,
	MRM_RECORD,
} MacroRunMode;

void macroInit( void );
void macroTerm( void );
void macroMain( HookCaller caller, SceCtrlData *pad_data, void *argp );
void macroIntr( const int mfengine );
MfMenuReturnCode macroMenu( SceCtrlLatch *pad_latch, SceCtrlData *pad_data, void *argp );

MfMenuReturnCode macroRunInterrupt( SceCtrlLatch *pad_latch, SceCtrlData *pad_data );
MfMenuReturnCode macroRunOnce( SceCtrlLatch *pad_latch, SceCtrlData *pad_data );
MfMenuReturnCode macroRunInfinity( SceCtrlLatch *pad_latch, SceCtrlData *pad_data );
MfMenuReturnCode macroLoad( SceCtrlLatch *pad_latch, SceCtrlData *pad_data );
MfMenuReturnCode macroEdit( SceCtrlLatch *pad_latch, SceCtrlData *pad_data );
MfMenuReturnCode macroClear( SceCtrlLatch *pad_latch, SceCtrlData *pad_data );
MfMenuReturnCode macroRecordStop( SceCtrlLatch *pad_latch, SceCtrlData *pad_data );
MfMenuReturnCode macroRecordStart( SceCtrlLatch *pad_latch, SceCtrlData *pad_data );

#endif

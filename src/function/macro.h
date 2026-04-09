
#ifndef __MACRO_H__
#define __MACRO_H__

#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include "psp/memsce.h"
#include "../menu.h"

#include "macrotypes.h"
#include "macroeditor.h"
	
typedef MfMenuReturnCode ( *MacroFunction )( SceCtrlLatch*, SceCtrlData* );

typedef enum {
	MRM_NONE = 0,
	MRM_TRACE,
	MRM_RECORD,
} MacroRunMode;

#define MACRO_NOTICE_DISPLAY_SEC 1.5

void macroInit( void );
void macroTerm( void );
void macroMain( HookCaller caller, SceCtrlData *pad_data, void *argp );
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

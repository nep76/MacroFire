/*
	Macro function
*/
#ifndef __MACRO_H__
#define __MACRO_H__

#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include "utils/uclc.h"
#include "psp/memsce.h"
#include "psp/cmndlg.h"
#include "psp/fileh.h"
#include "../menu.h"

#include "macromgr.h"
#include "macroeditor.h"

#define MACRO_MENU_OFFSET        4
#define MACRO_MENU_BASECONF      2
#define MACRO_NOTICE_DISPLAY_SEC 1.5
#define MACRO_ERROR_DISPLAY_SEC  3

#define MACRO_DATA_RECORD_MAXLEN    128
#define MACRO_DATA_RECORD_SEPARATOR "\t\x20"
#define MACRO_DATA_SIGNATURE    "MACROFIRE-MACRO"
#define MACRO_DATA_VERSION      1
#define MACRO_ACTION_DELAY      "DELAY"
#define MACRO_ACTION_BTNPRESS   "BUTTONSPRESS"
#define MACRO_ACTION_BTNRELEASE "BUTTONSRELEASE"
#define MACRO_ACTION_BTNCHANGE  "BUTTONSCHANGE"
#define MACRO_ACTION_ALNEUTRAL  "ANALOGNUETRAL"
#define MACRO_ACTION_ALMOVE     "ANALOGMOVE"

typedef MfMenuReturnCode ( *MacroFunction )( SceCtrlLatch*, SceCtrlData* );

typedef enum {
	MRM_NONE = 0,
	MRM_TRACE,
	MRM_RECORD,
} MacroRunMode;

void macroInit( void );
void macroTerm( void );
void macroMain( MfCallMode mode, SceCtrlData *pad_data, void *argp );
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

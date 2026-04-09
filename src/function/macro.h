/*
	Macro function
*/
#ifndef MACRO_H
#define MACRO_H

#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include "utils/strutil.h"
#include "psp/memsce.h"
#include "psp/cmndlg.h"
#include "psp/fileh.h"
#include "../menu.h"
#include "macromgr.h"
#include "macroeditor.h"

/*-----------------------------------------------
	íËêî
-----------------------------------------------*/
#define MACRO_RUN_ONCE      0
#define MACRO_RUN_INFINITY  1
#define MACRO_RUN_HALT      2
#define MACRO_RECORD_START  4
#define MACRO_RECORD_STOP   5
#define MACRO_ANALOG_OPTION 6
#define MACRO_EDIT          8
#define MACRO_CLEAR         9
#define MACRO_LOAD          11
#define MACRO_SAVE          12

#define MACRO_INFO_DISPLAY_MICROSEC  1000000
#define MACRO_ERROR_DISPLAY_MICROSEC 3000000

#define MACRO_DATA_SIGNATURE        "MACROFIRE-MACRO"
#define MACRO_DATA_VERSION          2

#define MACRO_ACTION_DELAY          "Delay"
#define MACRO_ACTION_BTNPRESS       "ButtonsPress"
#define MACRO_ACTION_BTNRELEASE     "ButtonsRelease"
#define MACRO_ACTION_BTNCHANGE      "ButtonsChange"
#define MACRO_ACTION_ALMOVE         "AnalogMove"

/*-----------------------------------------------
	å^êÈåæ
-----------------------------------------------*/
typedef MfMenuRc ( *MacroFunction )( SceCtrlData*, void* );

typedef enum {
	MRM_NONE = 0,
	MRM_TRACE,
	MRM_RECORD,
} MacroRunMode;

/*-----------------------------------------------
	ä÷êî
-----------------------------------------------*/
void macroInit( void );
void macroTerm( void );
void macroMain( MfCallMode mode, SceCtrlData *pad_data, void *argp );
void macroIntr( const bool mfengine );

MfMenuRc macroMenu        ( SceCtrlData *pad_data, void *arg );
MfMenuRc macroRunInterrupt( SceCtrlData *pad_data, void *arg );
MfMenuRc macroRunOnce     ( SceCtrlData *pad_data, void *arg );
MfMenuRc macroRunInfinity ( SceCtrlData *pad_data, void *arg );
MfMenuRc macroEdit        ( SceCtrlData *pad_data, void *arg );
MfMenuRc macroClear       ( SceCtrlData *pad_data, void *arg );
MfMenuRc macroCreate      ( SceCtrlData *pad_data, void *arg );
MfMenuRc macroRecordStop  ( SceCtrlData *pad_data, void *arg );
MfMenuRc macroRecordStart ( SceCtrlData *pad_data, void *arg );
MfMenuRc macroLoad        ( SceCtrlData *pad_data, void *arg );
MfMenuRc macroSave        ( SceCtrlData *pad_data, void *arg );

#endif

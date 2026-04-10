/*
	Macro function
*/
#ifndef MACRO_H
#define MACRO_H

#include <pspctrl.h>
#include "utils/strutil.h"
#include "psp/memsce.h"
#include "psp/cmndlg.h"
#include "psp/fileh.h"
#include "../macrofire.h"
#include "macromgr.h"
#include "macroeditor.h"

/*-----------------------------------------------
	íËêî
-----------------------------------------------*/
#define MACRO_MAX_SLOT      10

#define MACRO_SELECT_SLOT   0
#define MACRO_SET_TRIGGER   2
#define MACRO_RUN_ONCE      4
#define MACRO_RUN_INFINITY  5
#define MACRO_RUN_HALT      6
#define MACRO_RECORD_START  8
#define MACRO_RECORD_STOP   9
#define MACRO_ANALOG_OPTION 10
#define MACRO_EDIT          12
#define MACRO_CLEAR         13
#define MACRO_LOAD          15
#define MACRO_SAVE          16

#define MACRO_DATA_SIGNATURE        "MACROFIRE-MACRO"
#define MACRO_DATA_VERSION          3

#define MACRO_ACTION_DELAY          "Delay"
#define MACRO_ACTION_BTNPRESS       "ButtonsPress"
#define MACRO_ACTION_BTNRELEASE     "ButtonsRelease"
#define MACRO_ACTION_BTNCHANGE      "ButtonsChange"
#define MACRO_ACTION_ALMOVE         "AnalogMove"
#define MACRO_ACTION_BTNRAPIDSTART  "RapidfireStart"
#define MACRO_ACTION_BTNRAPIDSTOP   "RapidfireStop"

/*-----------------------------------------------
	É}ÉNÉç
-----------------------------------------------*/
#define MACRO_GET_ARG_BY_PTR( p ) MFM_GET_CB_ARG_BY_PTR( p, 0 )
#define MACRO_GET_ARG_BY_INT( p ) MFM_GET_CB_ARG_BY_INT( p, 0 )
#define MACRO_GET_ARG_BY_STR( p ) MFM_GET_CB_ARG_BY_STR( p, 0 )

/*-----------------------------------------------
	å^êÈåæ
-----------------------------------------------*/
typedef MfMenuRc ( *MacroFunction )( SceCtrlData*, void* );

enum macro_error_codes {
	MACRO_ERROR_NONE           = 0,
	MACRO_ERROR_BUSY           = 0x00000001,
	MACRO_ERROR_UNAVAIL        = 0x00000002,
	MACRO_ERROR_DISABLE_ENGINE = 0x00000004
};

struct macro_tempdata {
	unsigned int buttons;
	uint64_t     analogCoord;
	uint64_t     rtc;
};

enum macro_run_mode {
	MRM_NONE = 0,
	MRM_TRACE,
	MRM_RECORD,
};

struct macro_params {
	MacroData             *macro;
	MacroData             *command;
	enum macro_run_mode   runMode;
	unsigned int          runLoop;
	MfRapidfireUID        rfUid;
	struct macro_tempdata temp;
	bool                  hotkeyEnable;
};

typedef struct {
	unsigned int        runButtons;
	bool                analogStick;
	struct macro_params current;
} MacroEntry;

/*-----------------------------------------------
	ä÷êî
-----------------------------------------------*/
void macroInit( void );
void macroTerm( void );
void macroIntr( const bool mfengine );
void macroMain( MfCallMode mode, SceCtrlData *pad_data, void *argp );
MfMenuRc macroMenu( SceCtrlData *pad_data, void *arg );

#endif

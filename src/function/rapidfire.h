/*
	Rapidfire機能はベース機能に取り込まれたので、
	メインの関数はなくなった。
*/
#ifndef RAPIDFIRE_H
#define RAPIDFIRE_H

#include <pspctrl.h>
#include "utils/strutil.h"
#include "psp/cmndlg.h"
#include "psp/fileh.h"
#include "../macrofire.h"

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define RAPIDFIRE_DEFAULT_RELEASE_DELAY 18
#define RAPIDFIRE_DEFAULT_PRESS_DELAY   18

#define RAPIDFIRE_INFO_DISPLAY_MICROSEC  1000000
#define RAPIDFIRE_ERROR_DISPLAY_MICROSEC 3000000

#define RAPIDFIRE_BUTTON_CIRCLE    0
#define RAPIDFIRE_BUTTON_CROSS     1
#define RAPIDFIRE_BUTTON_SQUARE    2
#define RAPIDFIRE_BUTTON_TRIANGLE  3
#define RAPIDFIRE_BUTTON_UP        4
#define RAPIDFIRE_BUTTON_RIGHT     5
#define RAPIDFIRE_BUTTON_DOWN      6
#define RAPIDFIRE_BUTTON_LEFT      7
#define RAPIDFIRE_BUTTON_LTRIGGER  8
#define RAPIDFIRE_BUTTON_RTRIGGER  9
#define RAPIDFIRE_BUTTON_START    10
#define RAPIDFIRE_BUTTON_SELECT   11

#define RAPIDFIRE_MODE_NORMAL     0
#define RAPIDFIRE_MODE_SEMI_RAPID 1
#define RAPIDFIRE_MODE_AUTO_RAPID 2
#define RAPIDFIRE_MODE_AUTO_HOLD  3
#define RAPIDFIRE_LOAD 19
#define RAPIDFIRE_SAVE 20

#define RAPIDFIRE_DATA_SIGNATURE        "MACROFIRE-RAPIDFIRE"
#define RAPIDFIRE_DATA_VERSION          2
#define RAPIDFIRE_DID_CIRCLE            "Circle"
#define RAPIDFIRE_DID_CROSS             "Cross"
#define RAPIDFIRE_DID_SQUARE            "Square"
#define RAPIDFIRE_DID_TRIANGLE          "Triangle"
#define RAPIDFIRE_DID_UP                "Up"
#define RAPIDFIRE_DID_RIGHT             "Right"
#define RAPIDFIRE_DID_DOWN              "Down"
#define RAPIDFIRE_DID_LEFT              "Left"
#define RAPIDFIRE_DID_LTRIGGER          "L-Trigger"
#define RAPIDFIRE_DID_RTRIGGER          "R-Trigger"
#define RAPIDFIRE_DID_START             "START"
#define RAPIDFIRE_DID_SELECT            "SELECT"
#define RAPIDFIRE_DID_RDELAY            "ReleaseDelay"
#define RAPIDFIRE_DID_PDELAY            "PressDelay"

#define RAPIDFIRE_DID_MODENORMAL        "NORMAL"
#define RAPIDFIRE_DID_MODESEMIRAPID     "RAPID"
#define RAPIDFIRE_DID_MODEAUTORAPID     "AUTO-RAPID"
#define RAPIDFIRE_DID_MODEAUTOHOLD      "AUTO-HOLD"

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef MfMenuRc ( *RapidfireConfIo )( void );

typedef struct {
	unsigned int button;
	int  mode;
} RapidfireConf;

/*-----------------------------------------------
	関数
-----------------------------------------------*/
MfMenuRc rapidfireMenu( SceCtrlData *pad_data, void *argp );
void     rapidfireApply( void );

#endif

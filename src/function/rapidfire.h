/*
	Rapidfire function
*/
#ifndef __RAPIDFIRE_H__
#define __RAPIDFIRE_H__

#include <pspctrl.h>
#include "utils/strutil.h"
#include "psp/blit.h"
#include "psp/cmndlg.h"
#include "psp/fileh.h"
#include "../menu.h"

/*-----------------------------------------------
	íËêî
-----------------------------------------------*/
#define RAPIDFIRE_ERROR_DISPLAY_SEC 3

#define RAPIDFIRE_DATA_RECORD_MAXLEN    64
#define RAPIDFIRE_DATA_RECORD_SEPARATOR "\t\x20"
#define RAPIDFIRE_DATA_SIGNATURE        "MACROFIRE-RAPIDFIRE"
#define RAPIDFIRE_DATA_VERSION          1
#define RAPIDFIRE_DID_CIRCLE            "CIRCLE"
#define RAPIDFIRE_DID_CROSS             "CROSS"
#define RAPIDFIRE_DID_SQUARE            "SQUARE"
#define RAPIDFIRE_DID_TRIANGLE          "TRIANGLE"
#define RAPIDFIRE_DID_UP                "UP"
#define RAPIDFIRE_DID_RIGHT             "RIGHT"
#define RAPIDFIRE_DID_DOWN              "DOWN"
#define RAPIDFIRE_DID_LEFT              "LEFT"
#define RAPIDFIRE_DID_LTRIGGER          "LTRIGGER"
#define RAPIDFIRE_DID_RTRIGGER          "RTRIGGER"
#define RAPIDFIRE_DID_START             "START"
#define RAPIDFIRE_DID_SELECT            "SELECT"
#define RAPIDFIRE_DID_RDELAY            "RDELAY"
#define RAPIDFIRE_DID_PDELAY            "PDELAY"

/*-----------------------------------------------
	â◊íSêÈåæ
-----------------------------------------------*/
typedef struct {
	unsigned int button;
	int  mode;
} RapidfireConf;

/*-----------------------------------------------
	ä÷êî
-----------------------------------------------*/
void             rapidfireInit( ConfmgrHandler conf[] );
void             rapidfireMain( MfCallMode mode, SceCtrlData *pad_data, void *argp );
MfMenuReturnCode rapidfireMenu( SceCtrlLatch *pad_latch, SceCtrlData *pad_data, void *argp );

#endif

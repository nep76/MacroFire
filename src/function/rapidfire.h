
#ifndef __RAPIDFIRE_H__
#define __RAPIDFIRE_H__

#include <pspctrl.h>
#include "../menu.h"
#include "psp/blit.h"

typedef struct {
	unsigned int button;
	int  mode;
} RapidfireConf;

void rapidfireMain( HookCaller caller, SceCtrlData *pad_data, void *argp );
MfMenuReturnCode rapidfireMenu( SceCtrlLatch *pad_latch, SceCtrlData *pad_data, void *argp );

#endif

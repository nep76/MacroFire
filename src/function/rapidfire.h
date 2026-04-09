
#ifndef __RAPIDFIRE_H__
#define __RAPIDFIRE_H__

#include <pspctrl.h>
#include "../menu.h"

typedef struct {
	unsigned int button;
	int  mode;
} RapidfireConf;

bool rapidfireMain( HookCaller caller, SceCtrlData *pad_data, void *argp );
bool rapidfireMenu( SceCtrlLatch *pad_latch, SceCtrlData *pad_data, void *argp );

#endif

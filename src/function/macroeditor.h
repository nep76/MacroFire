/*
	macroeditor.h
	
	macro.c‚©‚çŒÄ‚Î‚ê‚é
*/

#ifndef __MACROEDITOR_H__
#define __MACROEDITOR_H__

#include <pspkernel.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <stdbool.h>
#include "psp/blit.h"
#include "sstring.h"
#include "../menu.h"

#include "macrotypes.h"

typedef MfMenuReturnCode ( *MacroeditorEditFunction )( SceCtrlLatch*, MacroData* );

MfMenuReturnCode macroeditorMain( SceCtrlLatch *pad_latch, SceCtrlData *pad_data, MacroData *macro );

#endif

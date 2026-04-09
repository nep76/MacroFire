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

#include "psp/cmndlg.h"
#include "macromgr.h"

#define MACROEDITOR_LINES_PER_PAGE 28

#define MACROEDITOR_MAINMENU_POS_X blitOffsetChar( 39 )
#define MACROEDITOR_MAINMENU_POS_Y blitOffsetLine( 5 )
#define MACROEDITOR_EDIT_BUTTONS_POS_X blitOffsetChar( 39 )
#define MACROEDITOR_EDIT_BUTTONS_POS_Y blitOffsetLine( 5 )
#define MACROEDITOR_EDIT_WAITMS_POS_X blitOffsetChar( 39 )
#define MACROEDITOR_EDIT_WAITMS_POS_Y blitOffsetLine( 5 )
#define MACROEDITOR_EDIT_TYPE_POS_X blitOffsetChar( 39 )
#define MACROEDITOR_EDIT_TYPE_POS_Y blitOffsetLine( 5 )
#define MACROEDITOR_OFFSET_X( o, c ) ( ( o ) + ( c ) * BLIT_CHAR_WIDTH )
#define MACROEDITOR_OFFSET_Y( o, l ) ( ( o ) + ( l ) * BLIT_CHAR_HEIGHT )

typedef MfMenuReturnCode ( *MacroeditorEditFunction )( SceCtrlLatch*, MacroData* );

MfMenuReturnCode macroeditorMain( SceCtrlLatch *pad_latch, SceCtrlData *pad_data, MacroData *macro );

#endif

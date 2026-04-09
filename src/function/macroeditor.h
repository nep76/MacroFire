/*
	macroeditor.h
	
	macro.cÇ©ÇÁåƒÇŒÇÍÇÈ
*/

#ifndef MACROEDITOR_H
#define MACROEDITOR_H

#include <pspkernel.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <stdbool.h>
#include "psp/blit.h"
#include "utils/strutil.h"
#include "../macrofire.h"
#include "psp/cmndlg.h"
#include "macromgr.h"

/*-----------------------------------------------
	íËêî
-----------------------------------------------*/
#define MACROEDITOR_LINES_PER_PAGE 26

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

#define MACROEDITOR_WAITMS_DIGIT 9
#define MACROEDITOR_COORD_DIGIT  3

#define MACROEDITOR_TYPE_DELAY           0
#define MACROEDITOR_TYPE_BUTTONS_PRESS   1
#define MACROEDITOR_TYPE_BUTTONS_RELEASE 2
#define MACROEDITOR_TYPE_BUTTONS_CHANGE  3
#define MACROEDITOR_TYPE_ANALOG_MOVE     4
#define MACROEDITOR_TYPE_RAPIDFIRE_START 5
#define MACROEDITOR_TYPE_RAPIDFIRE_STOP  6

#define MACROEDITOR_AVAIL_BUTTONS ( \
	PSP_CTRL_UP       | PSP_CTRL_RIGHT  | PSP_CTRL_DOWN     | PSP_CTRL_LEFT     |\
	PSP_CTRL_TRIANGLE | PSP_CTRL_CIRCLE | PSP_CTRL_CROSS    | PSP_CTRL_SQUARE   |\
	PSP_CTRL_START    | PSP_CTRL_SELECT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER \
)

/*-----------------------------------------------
	å^êÈåæ
-----------------------------------------------*/
typedef MfMenuRc ( *MacroeditorEditFunction )( SceCtrlData*, MacroData* );
typedef MfMenuRc ( *MacroeditorEditDialog )( char*, char*, int, void* );


/*-----------------------------------------------
	ä÷êî
-----------------------------------------------*/
MfMenuRc macroeditorMain( SceCtrlData *pad_data, MacroData *macro );

#endif

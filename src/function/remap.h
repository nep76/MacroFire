/*
	Remap function
*/

#ifndef REMAP_H
#define REMAP_H

#include <pspctrl.h>
#include "psp/cmndlg.h"
#include "utils/strutil.h"
#include "psp/fileh.h"
#include "../macrofire.h"

/*-----------------------------------------------
	íËêî
-----------------------------------------------*/
#define REMAP_LINES_PER_PAGE 22

#define REMAP_COLUMN_LIST 0
#define REMAP_COLUMN_MENU 1

#define REMAP_DATA_SECTION        "Remap"
#define REMAP_DATA_SIGNATURE      "MACROFIRE-REMAP"
#define REMAP_DATA_VERSION        1
#define REMAP_DATA_REFUSE_VERSION 0
#define REMAP_REAL_BUTTONS        "RealButtons"
#define REMAP_REMAP_BUTTONS       "RemapButtons"

#define REMAP_INILINE_ALL_COLUMNS 2
#define REMAP_INILINE_SEQ         0
#define REMAP_INILINE_TARGET      1

/*-----------------------------------------------
	É}ÉNÉç
-----------------------------------------------*/

/*-----------------------------------------------
	å^êÈåæ
-----------------------------------------------*/
typedef struct RemapData {
	unsigned int realButtons;
	unsigned int remapButtons;
	struct RemapData *next;
	struct RemapData *prev;
} RemapData;

typedef MfMenuRc ( *RemapExtFunc )( RemapData** );

/*-----------------------------------------------
	ä÷êî
-----------------------------------------------*/
void remapLoadIni( IniUID ini, char *buf, size_t len );
void remapCreateIni( IniUID ini, char *buf, size_t len );
void remapTerm( void );
void remapMain( MfCallMode mode, SceCtrlData *pad, void *argp );
MfMenuRc remapMenu( SceCtrlData *pad, void *arg );

#endif

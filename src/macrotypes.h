/*
	macrotypes.h
	
	macro.c/macroeditor.cÇ≈égÇ§å^êÈåæÅB
*/

#ifndef __MACROTYPES_H__
#define __MACROTYPES_H__

#define MACRO_MAX_DELAY          9999999999999999999ULL

typedef enum {
	MA_DELAY = 0,
	MA_BUTTONS_PRESS,
	MA_BUTTONS_RELEASE,
	MA_BUTTONS_CHANGE,
} MacroAction;

typedef struct _macro_data {
	MacroAction action;
	u64 data;
	struct _macro_data *next;
	struct _macro_data *prev;
} MacroData;

#endif

/*
	msg.h
*/

#ifndef __MSG_H__
#define __MSG_H__

#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdarg.h>
#include "psp/blit.h"
#define __CMNDLG_FUNCTION_EXPORT__
#include "psp/cmndlg.h"
#undef __CMNDLG_FUNCTION_EXPORT__

#define CMNDLG_MSG_X( a, b ) ( a + blitMeasureChar( b ) )
#define CMNDLG_MSG_Y( a, b ) ( a + blitMeasureLine( b ) )

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CMNDLG_MSG_YES,
	CMNDLG_MSG_NO
} CmndlgMsgRc;

typedef enum {
	CMNDLG_MSG_OPTION_YESNO = 0x00000001
} CmndlgMsgOption;

CmndlgMsgRc cmndlgMsg( unsigned int x, unsigned int y, CmndlgMsgOption opt, char *str );
CmndlgMsgRc cmndlgMsgf( unsigned int x, unsigned int y, CmndlgMsgOption opt, char *format, ... );

#ifdef __cplusplus
}
#endif

#endif

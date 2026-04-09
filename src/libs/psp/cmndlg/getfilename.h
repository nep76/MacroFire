/*
	getfilename.h
*/

#ifndef __GETFILENAME_H__
#define __GETFILENAME_H__

#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include "sstring.h"
#include "psp/blit.h"
#include "psp/dirh.h"
#define __CMNDLG_FUNCTION_EXPORT__
#include "psp/sosk.h"
#include "psp/cmndlg.h"
#include "psp/cmndlg/msg.h"
#undef __CMNDLG_FUNCTION_EXPORT__

#define CMNDLG_GETFILENAME_ITEMS_PER_PAGE 19

/* ダイアログの位置 */
#define CMNDLG_GETFILENAME_SX( x ) ( 10  + blitMeasureChar( x ) )
#define CMNDLG_GETFILENAME_SY( y ) ( 5   + blitMeasureLine( y ) )
#define CMNDLG_GETFILENAME_EX( x ) ( 470 - blitMeasureChar( x ) )
#define CMNDLG_GETFILENAME_EY( y ) ( 267 - blitMeasureLine( y ) )

#ifdef __cplusplus
extern "C" {
#endif

enum getfilename_mode {
	CMNDLG_GETFILENAME_MODE_SAVE,
	CMNDLG_GETFILENAME_MODE_OPEN
};

enum getfilename_focus {
	CMNDLG_GETFILENAME_FOCUS_LIST,
	CMNDLG_GETFILENAME_FOCUS_NAME
};

enum getfilename_control_rc {
	CMNDLG_GETFILENAME_CONTROL_RC_NOOP        = 0,
	CMNDLG_GETFILENAME_CONTROL_RC_QUIT        = 0x00000001,
	CMNDLG_GETFILENAME_CONTROL_RC_CONTINUE    = 0x00000010,
	CMNDLG_GETFILENAME_CONTROL_RC_REDRAW_LIST = 0x00000100,
	CMNDLG_GETFILENAME_CONTROL_RC_REDRAW_NAME = 0x00001000
};

enum getfilename_rc {
	CMNDLG_GETFILENAME_RC_CANCEL,
	CMNDLG_GETFILENAME_RC_ACCEPT,
	CMNDLG_GETFILENAME_RC_CONTINUE,
};

typedef enum {
	CMNDLG_GETFILENAME_CANCEL,
	CMNDLG_GETFILENAME_ACCEPT
} CmndlgGetFilenameRc;

typedef struct {
	char *dirPath;
	int  dirPathMax;
	char *fileName;
	int  fileNameMax;
} CmndlgOpenFilename;

CmndlgGetFilenameRc cmndlgGetSaveFilename( char *title, char *initdir, CmndlgOpenFilename *cofn );

CmndlgGetFilenameRc cmndlgGetOpenFilename( char *title, char *initdir, CmndlgOpenFilename *cofn );

#ifdef __cplusplus
}
#endif

#endif

/*
	Common dialog - Get filename
*/

#ifndef GETFILENAME_H
#define GETFILENAME_H

#include "psp/gb.h"
#include "psp/ctrlpad.h"
#include "psp/pathexpand.h"
#include "psp/dirh.h"
#define CMNDLG_FUNCTION_EXPORT
#include "psp/cmndlg.h"
#undef CMNDLG_FUNCTION_EXPORT
#include "psp/cmndlg/message.h"
#include "psp/cmndlg/sosk.h"


/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define CMNDLG_GET_FILENAME_MAX_WIDTH  ( SCR_WIDTH  / GB_CHAR_WIDTH  )
#define CMNDLG_GET_FILENAME_MAX_HEIGHT ( SCR_HEIGHT / GB_CHAR_HEIGHT )

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
enum cmndlg_get_filename_focus {
	CMNDLG_GET_FILENAME_FOCUS_LIST,
	CMNDLG_GET_FILENAME_FOCUS_NAME
};

struct cmndlg_get_filename_tempdata {
	int selected;
	enum cmndlg_get_filename_focus focus;
};

typedef enum {
	CGF_OPEN            = 0x00000001,
	CGF_SAVE            = 0x00000002,
	CGF_FILEMUSTEXIST   = 0x00000010, /* 未実装 */
	CGF_OVERWRITEPROMPT = 0x00000020, /* 未実装 */
	CGF_CREATEPROMPT    = 0x00000040  /* 未実装 */
} CmndlgGetFilenameFlags;

typedef struct {
	char         *title;
	unsigned int flags;
	char         *initPath;
	char         *path;
	size_t       pathMax;
	char         *name;
	size_t       nameMax;
} CmndlgGetFilenameData;

typedef struct {
	unsigned int dirColor;
	unsigned int filelistBorderColor;
	unsigned int filenameBorderColor;
} CmndlgGetFilenameExtraUI;

typedef struct {
	CmndlgGetFilenameData    *data;
	int                      numberOfData;
	int                      selectDataNumber;
	CmndlgRc                 rc;
	CmndlgGetFilenameExtraUI extraUi;
	CmndlgUI                 ui;
	CmndlgBaseParams         base; /* 内部で使用。何もしなくてよい。 */
} CmndlgGetFilenameParams;

/*-----------------------------------------------
	関数
-----------------------------------------------*/
CmndlgGetFilenameParams *cmndlgGetFilenameGetParams( void );
CmndlgState cmndlgGetFilenameGetStatus( void );
int cmndlgGetFilenameStart( CmndlgGetFilenameParams *params );
int cmndlgGetFilenameUpdate( void );
int cmndlgGetFilenameShutdownStart( void );

#ifdef __cplusplus
}
#endif

#endif

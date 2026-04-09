/*
	getbuttons.h
	
	複数のボタンのオンオフ設定を取得するダイアログ
*/

/*
	getdigits.h
	
	指定した数だけ任意の桁数の数値を取得するダイアログ
*/

#ifndef __CMNDLG_GETBUTTONS_H__
#define __CMNDLG_GETBUTTONS_H__

#include <pspkernel.h>
#include <pspdisplay.h>
#include "psp/blit.h"
#define __CMNDLG_FUNCTION_EXPORT__
#include "psp/cmndlg.h"
#undef __CMNDLG_FUNCTION_EXPORT__

#define CMNDLG_GETDIGITS_ALL_MASK 0xffffffff

#ifdef __cplusplus
extern "C" {
#endif

struct cmndlg_getbuttons_buttonstat {
	unsigned int button;
	bool stat;
};

struct cmndlg_getbuttons_savestat {
	unsigned int buttons;
	int selected;
};

typedef struct {
	char *title;
	unsigned int *buttons;
	unsigned int btnMask;
} CmndlgGetButtons;

int cmndlgGetButtons( unsigned int x, unsigned int y, CmndlgGetButtons cgb[], unsigned int count );

#ifdef __cplusplus
}
#endif

#endif

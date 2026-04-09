/*
	getdigits.h
	
	指定した数だけ任意の桁数の数値を取得するダイアログ
*/

#ifndef __CMNDLG_GETDIGITS_H__
#define __CMNDLG_GETDIGITS_H__

#include <pspkernel.h>
#include <pspdisplay.h>
#include <stdlib.h>
#include "psp/blit.h"
#define __CMNDLG_FUNCTION_EXPORT__
#include "psp/cmndlg.h"
#undef __CMNDLG_FUNCTION_EXPORT__

#define CMNDLG_GETDIGITS_MAX_DIGIT 9

#ifdef __cplusplus
extern "C" {
#endif

struct cmndlg_getdigits_savestat {
	char numBuf[CMNDLG_GETDIGITS_MAX_DIGIT + 1];
	char *numStart;
	int numMaxIndex;
	int selected;
};

typedef struct {
	char *title;
	char *unit;
	long *number;
	int numDigits;
} CmndlgGetDigits;

int cmndlgGetDigits( unsigned int x, unsigned int y, CmndlgGetDigits cgd[], unsigned int count );

#ifdef __cplusplus
}
#endif

#endif

/*
	Simplified On Screen Keyboard
	
	すんごく簡易的なスクリーンキーボード。
	日本語は使えない、7bit ASCIIのみ。
	実行したら入力終了まで制御は呼び出し元スレッドに戻らない。
*/

#ifndef __SOSK_H__
#define __SOSK_H__

#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdbool.h>
#include <string.h>
#include "utils/strutil.h"
#include "psp/blit.h"
#include "psp/memsce.h"
#include "psp/cmndlg.h"

#define SOSK_X( a, b ) ( a + blitMeasureChar( b ) )
#define SOSK_Y( a, b ) ( a + blitMeasureLine( b ) )

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SR_ERROR,
	SR_CANCEL,
	SR_ACCEPT
} SoskRc;

SoskRc soskRun(
	char *desc,
	unsigned int x,
	unsigned int y,
	u32 fgcolor,
	u32 bgcolor,
	u32 fccolor,
	char *input,
	size_t len
);

#ifdef __cplusplus
}
#endif

#endif

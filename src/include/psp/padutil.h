/*
	PSPのコントローラデータを処理する。
*/

#ifndef PADUTIL_H
#define PADUTIL_H

#include <pspkernel.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <math.h>
#include "psp/memsce.h"
#include "utils/strutil.h"

/*-----------------------------------------------
	マクロ
-----------------------------------------------*/
#define PADUTIL_CENTER_X   127
#define PADUTIL_CENTER_Y   127
#define PADUTIL_MAX_COORD  255
#define PADUTIL_MAX_RADIUS 182 /* 辺の長さ * 2の平方根 (小数点以下切り上げ) */

#define PADUTIL_INVALID_RIGHT_TRIANGLE_DEGREE 45
#define PADUTIL_DEGREE_TO_RADIAN( d )         ( ( d ) *  (3.14 / 180 ) )
#define PADUTIL_SQUARE( x )                   ( ( x ) * ( x ) )
#define PADUTIL_IN_DEADZONE( x, y, r )        ( PADUTIL_SQUARE( x ) + PADUTIL_SQUARE( y ) <= ( PADUTIL_SQUARE( r ) ) )

#ifdef __cplusplus
extern "C" {
#endif
/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef struct {
	unsigned int button;
	char *name;
} PadutilButtons;

/*
	アナログパッドの方向を格納。
	ボタンコードとして使用し、enum PspCtrlButtonsが使用していない空きビットを使用。
*/
enum padutil_analog_direction {
	PADUTIL_CTRL_ANALOG_UP    = 0x00000002,
	PADUTIL_CTRL_ANALOG_RIGHT = 0x00000004,
	PADUTIL_CTRL_ANALOG_DOWN  = 0x00000400,
	PADUTIL_CTRL_ANALOG_LEFT  = 0x00000800
};

enum padutil_options {
	PADUTIL_OPT_TOKEN_SP  = 0x00000001,
	PADUTIL_OPT_IGNORE_SP = 0x00000002,
	PADUTIL_OPT_CASE_SENS = 0x00000004,
};

/*-----------------------------------------------
	関数
-----------------------------------------------*/
char *padutilGetButtonNamesByCode( PadutilButtons *availbtn, unsigned int buttons, const char *delim, unsigned int opt, char *buf, size_t len );
unsigned int padutilGetButtonCodeByNames( PadutilButtons *availbtn, const char *names, const char *delim, unsigned int opt );
unsigned int padutilGetAnalogDirection( int x, int y, int deadzone );
void padutilSetAnalogDirection( unsigned int analog_direction, unsigned char *x, unsigned char *y );

#ifdef __cplusplus
}
#endif

#endif


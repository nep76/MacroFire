/*
	Analog stick sensitivity tuner
*/

#ifndef MFANALOGSENS_H
#define MFANALOGSENS_H

#include <pspctrl.h>
#include <stdlib.h>
#include <math.h>
#include "macrofire.h"

/*-----------------------------------------------
	定数/マクロ
-----------------------------------------------*/
#define ANALOGTUNE_INIT_X     128
#define ANALOGTUNE_INIT_Y     128
#define ANALOGTUNE_INIT_R      40
#define ANALOGTUNE_INIT_SENS  100

#define ANALOGTUNE_MAX_COORD  255
#define ANALOGTUNE_MAX_RADIUS 128
#define ANALOGTUNE_MAX_SENS   200

#define ANALOGTUNE_SQUARE( x )     ( ( x ) * ( x ) )

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef struct {
	int x, y, r;
} AnalogtuneDeadzone;

/*-----------------------------------------------
	関数プロトタイプ
-----------------------------------------------*/
void analogtuneLoadIni( IniUID ini );
void analogtuneCreateIni( IniUID ini );
void analogtuneTune( SceCtrlData *pad_data, void *argp );
MfMenuRc analogtuneMenu( SceCtrlData *pad_data, void *argp );

int analogtuneGetOriginX( void );
int analogtuneGetOriginY( void );
int analogtuneGetDeadzone( void );

#endif

/*
	Rapidfire API
*/

#ifndef MFRAPIDFIRE_H
#define MFRAPIDFIRE_H

#include <pspctrl.h>
#include "macrofire.h"

/*-----------------------------------------------
	É}ÉNÉç
-----------------------------------------------*/
/*-----------------------------------------------
	å^êÈåæ
-----------------------------------------------*/
typedef int MfRapidfireUID;

typedef enum {
	MF_RAPIDFIRE_MODE_NORMAL = 0,
	MF_RAPIDFIRE_MODE_RAPID,
	MF_RAPIDFIRE_MODE_AUTORAPID,
	MF_RAPIDFIRE_MODE_HOLD,
	MF_RAPIDFIRE_MODE_AUTOHOLD
} MfRapidfireMode;

struct mf_rapidfire_status {
	unsigned int    button;
	MfRapidfireMode mode;
	unsigned int    delayPress;
	unsigned int    delayRelease;
	unsigned int    statLast;
	bool            state;
};

/*-----------------------------------------------
	ä÷êî
-----------------------------------------------*/
MfRapidfireUID mfRapidfireNew( void );
void mfRapidfireDestroy( MfRapidfireUID uid );

void mfSetRapidfire( MfRapidfireUID uid, unsigned int buttons, MfRapidfireMode mode, unsigned int pdelay, unsigned int rdelay );
void mfClearRapidfire( MfRapidfireUID uid, unsigned int buttons );
void mfClearRapidfireAll( MfRapidfireUID uid );
void mfRapidfire( MfRapidfireUID uid, MfCallMode caller, SceCtrlData *pad_data );

#endif

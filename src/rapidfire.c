/*
	rapidfire.c
*/

#include "rapidfire.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static struct mf_rapidfire_status *mf_rapidfire_get_params( MfRapidfireUID uid );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
struct mf_rapidfire_status st_rfst[] = {
	{ PSP_CTRL_CIRCLE,   MF_RAPIDFIRE_MODE_NORMAL, 0, 0, 0, false },
	{ PSP_CTRL_CROSS,    MF_RAPIDFIRE_MODE_NORMAL, 0, 0, 0, false },
	{ PSP_CTRL_SQUARE,   MF_RAPIDFIRE_MODE_NORMAL, 0, 0, 0, false },
	{ PSP_CTRL_TRIANGLE, MF_RAPIDFIRE_MODE_NORMAL, 0, 0, 0, false },
	{ PSP_CTRL_UP,       MF_RAPIDFIRE_MODE_NORMAL, 0, 0, 0, false },
	{ PSP_CTRL_RIGHT,    MF_RAPIDFIRE_MODE_NORMAL, 0, 0, 0, false },
	{ PSP_CTRL_DOWN,     MF_RAPIDFIRE_MODE_NORMAL, 0, 0, 0, false },
	{ PSP_CTRL_LEFT,     MF_RAPIDFIRE_MODE_NORMAL, 0, 0, 0, false },
	{ PSP_CTRL_LTRIGGER, MF_RAPIDFIRE_MODE_NORMAL, 0, 0, 0, false },
	{ PSP_CTRL_RTRIGGER, MF_RAPIDFIRE_MODE_NORMAL, 0, 0, 0, false },
	{ PSP_CTRL_START,    MF_RAPIDFIRE_MODE_NORMAL, 0, 0, 0, false },
	{ PSP_CTRL_SELECT,   MF_RAPIDFIRE_MODE_NORMAL, 0, 0, 0, false }
};

/*=============================================*/

MfRapidfireUID mfRapidfireNew( void )
{
	struct mf_rapidfire_status *params = (struct mf_rapidfire_status *)memsceMalloc( sizeof( st_rfst ) );
	if( ! params ) return 0;
	
	memcpy( params, st_rfst, sizeof( st_rfst ) );
	mfClearRapidfireAll( (MfRapidfireUID)params );
	
	return (MfRapidfireUID)params;
}

void mfRapidfireDestroy( MfRapidfireUID uid )
{
	struct mf_rapidfire_status *params;
	
	if( uid == 0 ) return;
	params = mf_rapidfire_get_params( uid );
	
	memsceFree( params );
}

void mfSetRapidfire( MfRapidfireUID uid, unsigned int buttons, MfRapidfireMode mode, unsigned int pdelay, unsigned int rdelay )
{
	struct mf_rapidfire_status *params = mf_rapidfire_get_params( uid );
	int i;
	
	if( ! buttons ) return;
	
	for( i = 0; i < MF_ARRAY_NUM( st_rfst ); i++ ){
		if( buttons & params[i].button ){
			params[i].mode         = mode;
			params[i].delayPress   = pdelay;
			params[i].delayRelease = rdelay;
			params[i].delayLast    = 0;
			params[i].state        = false;
		}
	}
}

void mfClearRapidfire( MfRapidfireUID uid, unsigned int buttons )
{
	mfSetRapidfire( uid, buttons, MF_RAPIDFIRE_MODE_NORMAL, 0, 0 );
}

void mfClearRapidfireAll( MfRapidfireUID uid )
{
	mfClearRapidfire( uid, 0xFFFFFFFF );
}

void mfRapidfire( MfRapidfireUID uid, MfCallMode caller, SceCtrlData *pad_data )
{
	struct mf_rapidfire_status *params = mf_rapidfire_get_params( uid );
	int i;
	unsigned int delayms;
	uint64_t curms;
	sceRtcGetCurrentTick( &curms );
	
	curms /= 1000;
	
	for( i = 0; i < MF_ARRAY_NUM( st_rfst ); i++ ){
		if(
			( pad_data->Buttons & params[i].button && params[i].mode == MF_RAPIDFIRE_MODE_RAPID ) ||
			( params[i].mode == MF_RAPIDFIRE_MODE_AUTORAPID )
		){
			if( pad_data->Buttons & params[i].button && params[i].mode == MF_RAPIDFIRE_MODE_AUTORAPID ){
				pad_data->Buttons ^= params[i].button;
				continue;
			}
			
			delayms = (unsigned int)curms - params[i].delayLast;
			
			if( params[i].state ){
				pad_data->Buttons |= params[i].button;
				if( caller == MF_CALL_READ && delayms > params[i].delayPress ){
					params[i].delayLast = curms;
					params[i].state     = false;
				}
			} else{
				if( params[i].mode == MF_RAPIDFIRE_MODE_RAPID ) pad_data->Buttons ^= params[i].button;
				if( caller == MF_CALL_READ && delayms > params[i].delayRelease ){
					params[i].delayLast = curms;
					params[i].state     = true;
				}
			}
		} else if( params[i].mode == MF_RAPIDFIRE_MODE_AUTOHOLD ){
			if( ! ( pad_data->Buttons & params[i].button ) ){
				pad_data->Buttons |= params[i].button;
			} else{
				pad_data->Buttons ^= params[i].button;
			}
		}
	}
}

static struct mf_rapidfire_status *mf_rapidfire_get_params( MfRapidfireUID uid )
{
	if( uid == 0 ){
		return st_rfst;
	} else{
		return (struct mf_rapidfire_status *)uid;
	}
}

/*=========================================================

	mfrapidfire.c

	連射実装。

=========================================================*/
#include "mfrapidfire.h"

/*=========================================================
	ローカルマクロ
=========================================================*/

/*=========================================================
	ローカル型宣言
=========================================================*/
struct mf_rapidfire_pref {
	unsigned int    button;       /* 対象ボタン */
	MfRapidfireMode mode;         /* モード */
	unsigned int    pressDelay;   /* 連射時のディレイ */
	unsigned int    releaseDelay; /* 連射時のディレイ */
	bool            autorun;      /* ボタンが押下されていなくとも自動実行するかどうか */
	unsigned int    bitFlags;   /* モードに応じて必要なフラグがセットされる */
};

struct mf_rapidfire_params {
	uint64_t lastTick;
	struct mf_rapidfire_pref *pref;
};

struct mf_rapidfire_progress {
	unsigned int button;
	unsigned int mode;
};

enum mf_rapidfire_flags_hold {
	MF_RAPIDFIRE_FLAGS_HOLD_ENABLE     = 0x00000001,
	MF_RAPIDFIRE_FLAGS_HOLD_ISREALHOLD = 0x00000002
};

enum mf_rapidfire_flags_rapid {
	MF_RAPIDFIRE_FLAGS_RAPID_PRESS = 0x00000001,
};

/*=========================================================
	ローカル関数
=========================================================*/

/*=========================================================
	ローカル変数
=========================================================*/
static struct mf_rapidfire_progress st_progress[] = {
	{ PSP_CTRL_SELECT,   MF_RAPIDFIRE_MODE_NORMAL },
	{ PSP_CTRL_START,    MF_RAPIDFIRE_MODE_NORMAL },
	{ PSP_CTRL_UP,       MF_RAPIDFIRE_MODE_NORMAL },
	{ PSP_CTRL_RIGHT,    MF_RAPIDFIRE_MODE_NORMAL },
	{ PSP_CTRL_DOWN,     MF_RAPIDFIRE_MODE_NORMAL },
	{ PSP_CTRL_LEFT,     MF_RAPIDFIRE_MODE_NORMAL },
	{ PSP_CTRL_LTRIGGER, MF_RAPIDFIRE_MODE_NORMAL },
	{ PSP_CTRL_RTRIGGER, MF_RAPIDFIRE_MODE_NORMAL },
	{ PSP_CTRL_TRIANGLE, MF_RAPIDFIRE_MODE_NORMAL },
	{ PSP_CTRL_CIRCLE,   MF_RAPIDFIRE_MODE_NORMAL },
	{ PSP_CTRL_CROSS,    MF_RAPIDFIRE_MODE_NORMAL },
	{ PSP_CTRL_SQUARE,   MF_RAPIDFIRE_MODE_NORMAL }
};

/*=======================================================
	関数
=========================================================*/
MfRapidfireUID mfRapidfireNew( void )
{
	unsigned short i;
	struct mf_rapidfire_params *params = (struct mf_rapidfire_params *)memoryAlloc( sizeof( struct mf_rapidfire_params ) + sizeof( struct mf_rapidfire_pref ) * MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS );
	if( ! params ) return 0;
	
	params->lastTick = 0;
	params->pref     = (struct mf_rapidfire_pref *)( (uintptr_t)params + sizeof( struct mf_rapidfire_params ) );
	
	for( i = 0; i < MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS; i++ ){
		params->pref[i].button       = st_progress[i].button;
		params->pref[i].mode         = MF_RAPIDFIRE_MODE_NORMAL;
		params->pref[i].pressDelay   = 0;
		params->pref[i].releaseDelay = 0;
		params->pref[i].autorun      = false;
		params->pref[i].bitFlags     = 0;
	}
	
	return (MfRapidfireUID)params;
}

void mfRapidfireDestroy( MfRapidfireUID uid )
{
	memoryFree( (void *)uid );
}

void mfRapidfireSetRapid( MfRapidfireUID uid, unsigned int buttons, unsigned int pdelay, unsigned int rdelay, bool autorun )
{
	unsigned short i;
	struct mf_rapidfire_pref *pref = ((struct mf_rapidfire_params *)uid)->pref;
	
	for( i = 0; i < MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS; i++ ){
		if( buttons & pref[i].button ){
			pref[i].mode         = MF_RAPIDFIRE_MODE_RAPID;
			pref[i].pressDelay   = pdelay;
			pref[i].releaseDelay = rdelay;
			pref[i].autorun      = autorun;
			pref[i].bitFlags     = 0;
		}
	}
}

void mfRapidfireSetHold( MfRapidfireUID uid, unsigned int buttons, bool autorun )
{
	unsigned short i;
	struct mf_rapidfire_pref *pref = ((struct mf_rapidfire_params *)uid)->pref;
	
	for( i = 0; i < MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS; i++ ){
		if( buttons & pref[i].button ){
			pref[i].mode         = MF_RAPIDFIRE_MODE_HOLD;
			pref[i].pressDelay   = 0;
			pref[i].releaseDelay = 0;
			pref[i].autorun      = autorun;
			pref[i].bitFlags     = 0;
		}
	}
}

void mfRapidfireClear( MfRapidfireUID uid, unsigned int buttons )
{
	unsigned short i;
	struct mf_rapidfire_pref *pref = ((struct mf_rapidfire_params *)uid)->pref;
	
	for( i = 0; i < MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS; i++ ){
		if( buttons & pref[i].button ){
			pref[i].mode         = MF_RAPIDFIRE_MODE_NORMAL;
			pref[i].pressDelay   = 0;
			pref[i].releaseDelay = 0;
			pref[i].autorun      = false;
			pref[i].bitFlags     = 0;
		}
	}
}

bool mfRapidfireGetEntry( MfRapidfireUID uid, enum PspCtrlButtons button, MfRapidfireMode *mode, unsigned int *pdelay, unsigned int *rdelay, bool *autorun )
{
	unsigned short i;
	struct mf_rapidfire_pref *pref = ((struct mf_rapidfire_params *)uid)->pref;
	
	/* ボタンコードから配列の要素番号を取得 */
	switch( button ){
		case PSP_CTRL_SELECT:   i = 0;  break;
		case PSP_CTRL_START:    i = 1;  break;
		case PSP_CTRL_UP:       i = 2;  break;
		case PSP_CTRL_RIGHT:    i = 3;  break;
		case PSP_CTRL_DOWN:     i = 4;  break;
		case PSP_CTRL_LEFT:     i = 5;  break;
		case PSP_CTRL_LTRIGGER: i = 6;  break;
		case PSP_CTRL_RTRIGGER: i = 7;  break;
		case PSP_CTRL_TRIANGLE: i = 8;  break;
		case PSP_CTRL_CIRCLE:   i = 9;  break;
		case PSP_CTRL_CROSS:    i = 10; break;
		case PSP_CTRL_SQUARE:   i = 11; break;
		default: return false;
	}
	
	if( mode    ) *mode    = pref[i].mode;
	if( pdelay  ) *pdelay  = pref[i].pressDelay;
	if( rdelay  ) *rdelay  = pref[i].releaseDelay;
	if( autorun ) *autorun = pref[i].autorun;
	
	return true;
}

bool mfRapidfireReadEntry( MfRapidfireUID uid, unsigned int *button, MfRapidfireMode *mode, unsigned int *pdelay, unsigned int *rdelay, bool *autorun, unsigned short *save )
{
	struct mf_rapidfire_pref *pref = ((struct mf_rapidfire_params *)uid)->pref;
	
	if( ! save || *save >= MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS ) return false;
	
	if( button  ) *button  = pref[*save].button;
	if( mode    ) *mode    = pref[*save].mode;
	if( pdelay  ) *pdelay  = pref[*save].pressDelay;
	if( rdelay  ) *rdelay  = pref[*save].releaseDelay;
	if( autorun ) *autorun = pref[*save].autorun;
	
	(*save)++;
	
	return true;
}

void mfRapidfireExec( MfRapidfireUID uid, MfHookAction action, SceCtrlData *pad )
{
	unsigned short i;
	uint64_t cur_tick;
	unsigned int delay;
	struct mf_rapidfire_params *params = (struct mf_rapidfire_params *)uid;
	
	sceRtcGetCurrentTick( &cur_tick );
	delay = ( cur_tick - params->lastTick ) / 1000;
	
	if( mfIsRunningApp( MF_APP_WEBBROWSER ) ) return;
	
	for( i = 0; i < MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS; i++ ){
		if( params->pref[i].mode == MF_RAPIDFIRE_MODE_HOLD && st_progress[i].mode < MF_RAPIDFIRE_MODE_HOLD ){
			if( params->pref[i].autorun ){
				pad->Buttons ^= params->pref[i].button;
			} else{
				if( pad->Buttons & params->pref[i].button ){
					if( ! ( params->pref[i].bitFlags & MF_RAPIDFIRE_FLAGS_HOLD_ISREALHOLD ) ){
						params->pref[i].bitFlags ^= MF_RAPIDFIRE_FLAGS_HOLD_ENABLE;
						params->pref[i].bitFlags |= MF_RAPIDFIRE_FLAGS_HOLD_ISREALHOLD;
					}
				} else{
					params->pref[i].bitFlags &= ~MF_RAPIDFIRE_FLAGS_HOLD_ISREALHOLD;
				}
				
				if( params->pref[i].bitFlags & MF_RAPIDFIRE_FLAGS_HOLD_ENABLE ){
					pad->Buttons |= params->pref[i].button;
				} else if( params->pref[i].bitFlags & MF_RAPIDFIRE_FLAGS_HOLD_ISREALHOLD ){
					pad->Buttons ^= params->pref[i].button;
				}
			}
			st_progress[i].mode = MF_RAPIDFIRE_MODE_HOLD;
		} else if( params->pref[i].mode == MF_RAPIDFIRE_MODE_RAPID && st_progress[i].mode < MF_RAPIDFIRE_MODE_RAPID ){
			uint64_t cur_tick;
			sceRtcGetCurrentTick( &cur_tick );
			
			if( action == MF_UPDATE && ( delay > ( ( params->pref[i].bitFlags & MF_RAPIDFIRE_FLAGS_RAPID_PRESS ) ? params->pref[i].pressDelay : params->pref[i].releaseDelay ) ) ){
				params->lastTick = cur_tick;
				params->pref[i].bitFlags ^= MF_RAPIDFIRE_FLAGS_RAPID_PRESS;
			}
			
			if( params->pref[i].autorun || ( pad->Buttons & params->pref[i].button ) ){
				pad->Buttons |= params->pref[i].button;
				if( ! ( params->pref[i].bitFlags & MF_RAPIDFIRE_FLAGS_RAPID_PRESS ) ){
					pad->Buttons ^= params->pref[i].button;
				}
			}
			st_progress[i].mode = MF_RAPIDFIRE_MODE_RAPID;
		}
	}
}

void mfRapidfireReset( void )
{
	st_progress[0].mode  = MF_RAPIDFIRE_MODE_NORMAL;
	st_progress[1].mode  = MF_RAPIDFIRE_MODE_NORMAL;
	st_progress[2].mode  = MF_RAPIDFIRE_MODE_NORMAL;
	st_progress[3].mode  = MF_RAPIDFIRE_MODE_NORMAL;
	st_progress[4].mode  = MF_RAPIDFIRE_MODE_NORMAL;
	st_progress[5].mode  = MF_RAPIDFIRE_MODE_NORMAL;
	st_progress[6].mode  = MF_RAPIDFIRE_MODE_NORMAL;
	st_progress[7].mode  = MF_RAPIDFIRE_MODE_NORMAL;
	st_progress[8].mode  = MF_RAPIDFIRE_MODE_NORMAL;
	st_progress[9].mode  = MF_RAPIDFIRE_MODE_NORMAL;
	st_progress[10].mode = MF_RAPIDFIRE_MODE_NORMAL;
	st_progress[11].mode = MF_RAPIDFIRE_MODE_NORMAL;
}

void mfRapidfireGetModeByName( const char *str, MfRapidfireMode *mode, bool *autorun )
{
	if( strcasecmp( str, MF_RAPIDFIRE_NAME_RAPID ) == 0 ){
		*mode    = MF_RAPIDFIRE_MODE_RAPID;
		*autorun = false;
	} else if( strcasecmp( str, MF_RAPIDFIRE_NAME_AUTORAPID ) == 0 ){
		*mode    = MF_RAPIDFIRE_MODE_RAPID;
		*autorun = true;
	} else if( strcasecmp( str, MF_RAPIDFIRE_NAME_HOLD ) == 0 ){
		*mode    = MF_RAPIDFIRE_MODE_HOLD;
		*autorun = false;
	} else if( strcasecmp( str, MF_RAPIDFIRE_NAME_HOLD ) == 0 ){
		*mode    = MF_RAPIDFIRE_MODE_HOLD;
		*autorun = true;
	} else{
		*mode    = MF_RAPIDFIRE_MODE_NORMAL;
		*autorun = false;
	}
}

const char *mfRapidfireGetNameByMode( MfRapidfireMode mode, bool autorun )
{
	switch( mode ){
		case MF_RAPIDFIRE_MODE_RAPID:
			return autorun ? MF_RAPIDFIRE_NAME_AUTORAPID : MF_RAPIDFIRE_NAME_RAPID;
		case MF_RAPIDFIRE_MODE_HOLD:
			return autorun ? MF_RAPIDFIRE_NAME_AUTOHOLD : MF_RAPIDFIRE_NAME_HOLD;
		default:
			return MF_RAPIDFIRE_NAME_NORMAL;
	}
}

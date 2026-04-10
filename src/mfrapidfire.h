/*=========================================================

	mfrapidfire.h

	連射実装。

=========================================================*/
#ifndef MFRAPIDFIRE_H
#define MFRAPIDFIRE_H

#include "macrofire.h"
#include <psprtc.h>

/*=========================================================
	マクロ
=========================================================*/
#define MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS 12

#define MF_RAPIDFIRE_DEFAULT_PRESS_DELAY   17
#define MF_RAPIDFIRE_DEFAULT_RELEASE_DELAY 17

#define MF_RAPIDFIRE_NAME_NORMAL    "Normal"
#define MF_RAPIDFIRE_NAME_RAPID     "Rapid"
#define MF_RAPIDFIRE_NAME_HOLD      "Hold"
#define MF_RAPIDFIRE_NAME_AUTORAPID "Auto-Rapid"
#define MF_RAPIDFIRE_NAME_AUTOHOLD  "Auto-Hold"

/*=========================================================
	型宣言
=========================================================*/
typedef intptr_t MfRapidfireUID;

typedef enum {
	MF_RAPIDFIRE_MODE_NORMAL = 0,
	MF_RAPIDFIRE_MODE_RAPID,
	MF_RAPIDFIRE_MODE_HOLD
} MfRapidfireMode;

typedef struct {
	unsigned int    button;       /* 対象ボタン */
	MfRapidfireMode mode;         /* モード */
	unsigned int    pressDelay;   /* 連射時のディレイ */
	unsigned int    releaseDelay; /* 連射時のディレイ */
	unsigned int    nextDelay;    /* 次のボタンアクションまでの待ち時間 */
	bool            autorun;      /* ボタンが押下されていなくとも自動実行するかどうか */
	unsigned int    bitFlags;     /* モードに応じて必要なフラグがセットされる */
} MfRapidfireData;

typedef struct {
	uint64_t lastTick;
	MfRapidfireData pref[MF_RAPIDFIRE_NUMBER_OF_AVAIL_BUTTONS];
} MfRapidfireParams;

/*=========================================================
	関数
=========================================================*/
MfRapidfireUID mfRapidfireNew( void );
void mfRapidfireDestroy( MfRapidfireUID uid );
MfRapidfireUID mfRapidfireBind( MfRapidfireParams *params );
void mfRapidfireSetRapid( MfRapidfireUID uid, unsigned int buttons, unsigned int pdelay, unsigned int rdelay, bool autorun );
void mfRapidfireSetHold( MfRapidfireUID uid, unsigned int buttons, bool autorun );
void mfRapidfireClear( MfRapidfireUID uid, unsigned int buttons );
bool mfRapidfireGetEntry( MfRapidfireUID uid, enum PspCtrlButtons button, MfRapidfireMode *mode, unsigned int *pdelay, unsigned int *rdelay, bool *autorun );
bool mfRapidfireReadEntry( MfRapidfireUID uid, unsigned int *button, MfRapidfireMode *mode, unsigned int *pdelay, unsigned int *rdelay, bool *autorun, unsigned short *save );
void mfRapidfireExec( MfRapidfireUID uid, MfHookAction action, SceCtrlData *pad );
void mfRapidfireReset( void );
void mfRapidfireGetModeByName( const char *str, MfRapidfireMode *mode, bool *autorun );
const char *mfRapidfireGetNameByMode( MfRapidfireMode mode, bool autorun );

#endif

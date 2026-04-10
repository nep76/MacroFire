/*=========================================================

	mfdialog.c

	ダイアログラッパー

=========================================================*/
#include "mfdialog_internal.h"
#include "util/strutil.h"

/*=========================================================
	ローカル関数
=========================================================*/
static bool mfdialog_draw( CdialogStatus ( *statf )( void ), int ( *updatef )( void ) );
static bool mfdialog_result( int ( *shutdownf )( void ), CdialogResult ( *resultf )( void ), void ( *destroyf )( void ) );

/*=========================================================
	ローカル変数
=========================================================*/
static MfDialogType st_dialog_type   = MF_DIALOG_NONE;
static bool st_last_result = true;
static bool st_getfilename_timeout = false; /* 必要性調査 */

/*=========================================================
	関数
=========================================================*/
inline void mfDialogInit( PadutilRemap *remap )
{
	cdialogInit();
	cdialogSetRemap( remap );
}

inline MfDialogType mfDialogCurrentType( void )
{
	return st_dialog_type;
}

inline void mfDialogFinish( void )
{
	cdialogClearRemap();
	cdialogFinish();
}

inline bool mfDialogLastResult( void )
{
	return st_last_result;
}

/*-----------------------------------------------
	メッセージダイアログ
-----------------------------------------------*/
bool mfDialogMessage( const char *title, const char *message, unsigned int error, bool yesno )
{
	CdialogMessageData *data;
	
	if( st_dialog_type != MF_DIALOG_NONE || cdialogMessageInit( NULL ) < 0 ) return false;
	
	dbgprint( "Init dialog: message" );
	
	data = cdialogMessageGetData();
	if( title   ) strutilCopy( data->title,   title,   CDIALOG_MESSAGE_TITLE_LENGTH );
	if( message ) strutilCopy( data->message, message, CDIALOG_MESSAGE_LENGTH );
	data->options = CDIALOG_DISPLAY_CENTER;
	if( error ){
		data->options   |= CDIALOG_MESSAGE_ERROR;
		data->errorcode = error;
	}
	if( yesno ) data->options |= CDIALOG_MESSAGE_YESNO;
	
	if( cdialogMessageStart( 0, 0 ) < 0 ){
		cdialogMessageShutdownStart();
		return false;
	}
	
	mfMenuDisable( MF_MENU_EXIT );
	st_dialog_type = MF_DIALOG_MESSAGE;
	
	return true;
}

bool mfDialogMessageDraw( void )
{
	return mfdialog_draw( cdialogMessageGetStatus, cdialogMessageUpdate );
}

bool mfDialogMessageResult( void )
{
	return mfdialog_result( cdialogMessageShutdownStart, cdialogMessageGetResult, cdialogMessageDestroy );
}

/*-----------------------------------------------
	SOSK
-----------------------------------------------*/
bool mfDialogSosk( const char *title, char *text, size_t length, unsigned int availkb )
{
	CdialogSoskData *data;
	
	if( st_dialog_type != MF_DIALOG_NONE || cdialogSoskInit( NULL ) < 0 ) return false;
	
	dbgprint( "Init dialog: sosk" );
	
	data = cdialogSoskGetData();
	if( title ) strutilCopy( data->title, title, CDIALOG_SOSK_TITLE_LENGTH );
	data->text    = text;
	data->textMax = length;
	data->types   = availkb;
	data->options = CDIALOG_DISPLAY_CENTER;
	
	if( cdialogSoskStart( 0, 0 ) < 0 ){
		cdialogSoskShutdownStart();
		return false;
	}
	
	mfMenuDisable( MF_MENU_EXIT );
	st_dialog_type = MF_DIALOG_SOSK;
	
	return true;
}

bool mfDialogSoskDraw( void )
{
	return mfdialog_draw( cdialogSoskGetStatus, cdialogSoskUpdate );
}

bool mfDialogSoskResult( void )
{
	return mfdialog_result( cdialogSoskShutdownStart, cdialogSoskGetResult, cdialogSoskDestroy );
}

/*-----------------------------------------------
	数値
-----------------------------------------------*/
bool mfDialogNumedit( const char *title, const char *unit, void *num, uint32_t max )
{
	CdialogNumeditData *data;
	
	if( st_dialog_type != MF_DIALOG_NONE || cdialogNumeditInit( NULL ) < 0 ) return false;
	
	dbgprint( "Init dialog: numedit" );
	
	data = cdialogNumeditGetData();
	if( title ) strutilCopy( data->title, title, CDIALOG_NUMEDIT_TITLE_LENGTH );
	if( unit  ) strutilCopy( data->unit,  unit,  CDIALOG_NUMEDIT_UNIT_LENGTH );
	data->num = num;
	data->max = max;
	data->options = CDIALOG_DISPLAY_CENTER;
	
	if( cdialogNumeditStart( 0, 0 ) < 0 ){
		cdialogNumeditShutdownStart();
		return false;
	}
	
	mfMenuDisable( MF_MENU_EXIT );
	st_dialog_type = MF_DIALOG_NUMEDIT;
	
	return true;
}

bool mfDialogNumeditDraw( void )
{
	return mfdialog_draw( cdialogNumeditGetStatus, cdialogNumeditUpdate );
}

bool mfDialogNumeditResult( void )
{
	return mfdialog_result( cdialogNumeditShutdownStart, cdialogNumeditGetResult, cdialogNumeditDestroy );
}

/*-----------------------------------------------
	ファイル名取得
-----------------------------------------------*/
bool mfDialogGetfilename( const char *title, const char *initdir, const char *initname, char *path, size_t pathmax, unsigned int options )
{
	CdialogGetfilenameData *data;
	
	if( st_dialog_type != MF_DIALOG_NONE || cdialogGetfilenameInit( NULL ) < 0 ) return false;
	
	dbgprint( "Init dialog: getfilename" );
	
	st_getfilename_timeout = false;
	
	data = cdialogGetfilenameGetData();
	if( title ) strutilCopy( data->title, title, CDIALOG_GETFILENAME_TITLE_LENGTH );
	data->initialDir  = initdir;
	data->initialName = initname;
	data->path        = path;
	data->pathMax     = pathmax;
	data->options     = CDIALOG_DISPLAY_CENTER | options;
	
	if( cdialogGetfilenameStart( 0, 0 ) < 0 ){
		cdialogGetfilenameShutdownStart();
		return false;
	}
	
	mfMenuDisable( MF_MENU_EXIT );
	st_dialog_type = MF_DIALOG_GETFILENAME;
	
	return true;
}

bool mfDialogGetfilenameDraw( void )
{
	CdialogStatus status;
	
	if( st_getfilename_timeout || cdialogGetfilenameUpdate() < 0 ){
		CdialogStatus errormsg = cdialogMessageGetStatus();
		st_getfilename_timeout = true;
		
		if( errormsg == CDIALOG_NONE ){
			CdialogMessageData *data;
			
			if( cdialogMessageInit( NULL ) < 0 ) return false;
			data = cdialogMessageGetData();
			strutilCopy( data->title,   "Read timed out", CDIALOG_MESSAGE_TITLE_LENGTH );
			strutilCopy( data->message, "MemoryStick is currently busy.\nPlease try again later.", CDIALOG_MESSAGE_LENGTH );
			data->options = CDIALOG_DISPLAY_CENTER;
			
			if( cdialogMessageStartNoLock( 0, 0 ) < 0 ){
				cdialogMessageShutdownStart();
				return false;
			}
		} else if( errormsg == CDIALOG_VISIBLE ){
			cdialogMessageUpdate();
		} else if( errormsg == CDIALOG_SHUTDOWN ){
			cdialogMessageShutdownStart();
			cdialogMessageDestroy();
			return false;
		}
		return true;
	}
	
	status = cdialogGetfilenameGetStatus();
	if( status == CDIALOG_SHUTDOWN ){
		return false;
	} else{
		return true;
	}
}

bool mfDialogGetfilenameResult( void )
{
	return mfdialog_result( cdialogGetfilenameShutdownStart, cdialogGetfilenameGetResult, cdialogGetfilenameDestroy );
}


/*-----------------------------------------------
	ボタン取得
-----------------------------------------------*/
bool mfDialogDetectbuttons( const char *title, PadutilButtons availbtns, PadutilButtons *buttons )
{
	CdialogDetectbuttonsData *data;
	
	if( st_dialog_type != MF_DIALOG_NONE || cdialogDetectbuttonsInit( NULL ) < 0 ) return false;
	
	dbgprint( "Init dialog: detectbuttons" );
	
	data = cdialogDetectbuttonsGetData();
	if( title ) strutilCopy( data->title, title, CDIALOG_DETECTBUTTONS_TITLE_LENGTH );
	data->availButtons = availbtns;
	data->buttons      = buttons;
	data->options      = CDIALOG_DISPLAY_CENTER;
	
	if( cdialogDetectbuttonsStart( 0, 0 ) < 0 ){
		cdialogDetectbuttonsShutdownStart();
		return false;
	}
	
	mfMenuDisable( MF_MENU_EXIT );
	st_dialog_type = MF_DIALOG_DETECTBUTTONS;
	
	return true;
}

bool mfDialogDetectbuttonsDraw( void )
{
	return mfdialog_draw( cdialogDetectbuttonsGetStatus, cdialogDetectbuttonsUpdate );
}

bool mfDialogDetectbuttonsResult( void )
{
	return mfdialog_result( cdialogDetectbuttonsShutdownStart, cdialogDetectbuttonsGetResult, cdialogDetectbuttonsDestroy );
}

/*=========================================================
	ローカル関数
=========================================================*/
static bool mfdialog_draw( CdialogStatus ( *statf )( void ), int ( *updatef )( void ) )
{
	CdialogStatus status;
	
	updatef();
	
	status = statf();
	if( status == CDIALOG_SHUTDOWN ){
		return false;
	} else{
		return true;
	}
}

static bool mfdialog_result( int ( *shutdownf )( void ), CdialogResult ( *resultf )( void ), void ( *destroyf )( void ) )
{
	CdialogResult result;
	
	shutdownf();
	result = resultf();
	
	destroyf();
	
	mfMenuEnable( MF_MENU_EXIT );
	st_dialog_type = MF_DIALOG_NONE;
	
	st_last_result = result == CDIALOG_ACCEPT ? true : false;
	
	return st_last_result;
}

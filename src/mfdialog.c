/*=========================================================

	mfdialog.c

	ダイアログラッパー

=========================================================*/
#include "mfdialog.h"

/*=========================================================
	ローカル関数
=========================================================*/
bool mfdialog_draw( CdialogStatus ( *statf )( void ), int ( *updatef )( void ) );
bool mfdialog_result( int ( *shutdownf )( void ), CdialogResult ( *resultf )( void ), void ( *destroyf )( void ) );

/*=========================================================
	ローカル変数
=========================================================*/
static MfDialogType st_dialog_type = MFDIALOG_NONE;
static bool st_getfilename_timeout = false;

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

/*-----------------------------------------------
	メッセージダイアログ
-----------------------------------------------*/
bool mfDialogMessageInit( const char *title, const char *message, bool yesno )
{
	CdialogMessageData *data;
	
	if( st_dialog_type != MFDIALOG_NONE || cdialogMessageInit( NULL ) < 0 ) return false;
	
	dbgprint( "Init dialog: message" );
	
	data = cdialogMessageGetData();
	if( title   ) strutilCopy( data->title,   title,   CDIALOG_MESSAGE_TITLE_LENGTH );
	if( message ) strutilCopy( data->message, message, CDIALOG_MESSAGE_LENGTH );
	data->options = CDIALOG_DISPLAY_CENTER;
	if( yesno ) data->options |= CDIALOG_MESSAGE_YESNO;
	
	if( cdialogMessageStart( 0, 0 ) < 0 ){
		cdialogMessageShutdownStart();
		return false;
	}
	
	mfMenuDisableQuickQuit();
	st_dialog_type = MFDIALOG_MESSAGE;
	
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
bool mfDialogSoskInit( const char *title, char *text, size_t length, unsigned int availkb )
{
	CdialogSoskData *data;
	
	if( st_dialog_type != MFDIALOG_NONE || cdialogSoskInit( NULL ) < 0 ) return false;
	
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
	
	mfMenuDisableQuickQuit();
	st_dialog_type = MFDIALOG_SOSK;
	
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
bool mfDialogNumeditInit( const char *title, const char *unit, void *num, uint32_t max )
{
	CdialogNumeditData *data;
	
	if( st_dialog_type != MFDIALOG_NONE || cdialogNumeditInit( NULL ) < 0 ) return false;
	
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
	
	mfMenuDisableQuickQuit();
	st_dialog_type = MFDIALOG_NUMEDIT;
	
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
bool mfDialogGetfilenameInit( const char *title, const char *initdir, const char *initname, char *path, size_t pathmax, unsigned int options )
{
	CdialogGetfilenameData *data;
	
	if( st_dialog_type != MFDIALOG_NONE || cdialogGetfilenameInit( NULL ) < 0 ) return false;
	
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
	
	mfMenuDisableQuickQuit();
	st_dialog_type = MFDIALOG_GETFILENAME;
	
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
bool mfDialogDetectbuttonsInit( const char *title, PadutilButtons availbtns, PadutilButtons *buttons )
{
	CdialogDetectbuttonsData *data;
	
	if( st_dialog_type != MFDIALOG_NONE || cdialogDetectbuttonsInit( NULL ) < 0 ) return false;
	
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
	
	mfMenuDisableQuickQuit();
	st_dialog_type = MFDIALOG_DETECTBUTTONS;
	
	return true;
}

bool mfDialogDetectbuttonsDraw( void )
{
	return mfdialog_draw( cdialogDetectbuttonsGetStatus, cdialogDetectbuttonsUpdate );
}

bool mfDialogDetectbuttonsResult( void )
{
	dbgprint( "DETECTBUTTON FINISH" );
	return mfdialog_result( cdialogDetectbuttonsShutdownStart, cdialogDetectbuttonsGetResult, cdialogDetectbuttonsDestroy );
}

/*=========================================================
	ローカル関数
=========================================================*/
bool mfdialog_draw( CdialogStatus ( *statf )( void ), int ( *updatef )( void ) )
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

bool mfdialog_result( int ( *shutdownf )( void ), CdialogResult ( *resultf )( void ), void ( *destroyf )( void ) )
{
	CdialogResult result;
	
	shutdownf();
	result = resultf();
	
	destroyf();
	
	mfMenuEnableQuickQuit();
	st_dialog_type = MFDIALOG_NONE;
	
	return result == CDIALOG_ACCEPT ? true : false;
}

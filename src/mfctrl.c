/*=========================================================

	mfctrl.c

	組み込みコントロール。

=========================================================*/
#include "mfctrl.h"

/*-----------------------------------------------
	ローカル定数
-----------------------------------------------*/
#define MF_CTRL_DEF_INI_SIG_ACCEPT MF_SM_1
#define MF_CTRL_DEF_INI_SIG_FINISH MF_SM_2

/*-----------------------------------------------
	ローカル型宣言
-----------------------------------------------*/
struct mf_ctrl_ini_params {
	MfCtrlDefIniProc proc;
	MfCtrlDefIniPref *pref;
	MfCtrlDefIniAction action;
	char *path;
};

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static void mf_ctrl_ini_handler( MfMessage message, void *arg );
static void mf_ctrl_ini_error( int error );

/*-----------------------------------------------
	関数
-----------------------------------------------*/
void mfCtrlSetLabel( void *arg, char *format, ... )
{
	va_list ap;
	
	va_start( ap, format );
	vsnprintf( (char *)arg, MF_CTRL_BUFFER_LENGTH, format, ap );
	va_end( ap );
}

bool mfCtrlDefBool( MfMessage message, const char *label, void *var, void *unused, void *ex )
{
	switch( mfMenuMaskMainMessage( message ) ){
		case MF_CM_INIT:
		case MF_CM_TERM:
			break;
		case MF_CM_LABEL:
			mfCtrlSetLabel( ex, "%s: %s", label, *((bool *)var) ? MF_STR_CTRL_ON : MF_STR_CTRL_OFF );
			break;
		case MF_CM_INFO:
			mfMenuSetInfoText( *((unsigned int *)ex) | MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(%s/%s)%s", PB_SYM_PSP_SQUARE, mfMenuAcceptSymbol(), MF_STR_CTRL_TOGGLE_SWITCH );
			break;
		case MF_CM_PROC:
			if( mfMenuIsAnyPressed( mfMenuAcceptButton() | PSP_CTRL_SQUARE ) ) *((bool *)var) = *((bool *)var) ? false : true;
			break;
	}
	return true;
}

bool mfCtrlDefOptions( MfMessage message, const char *label, void *var, void *items, void *ex )
{
	switch( mfMenuMaskMainMessage( message ) ){
		case MF_CM_INIT:
		case MF_CM_TERM:
			 break;
		case MF_CM_LABEL:
			if( items ){
				mfCtrlSetLabel( ex, "%s: %s", label, ((char **)items)[*((unsigned int *)var)] );
			} else{
				mfCtrlSetLabel( ex, "%s: Option%d", label, *((unsigned int *)var) );
			}
			break;
		case MF_CM_INFO:
			mfMenuSetInfoText( *((unsigned int *)ex) | MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(%s/%s)%s", PB_SYM_PSP_SQUARE, mfMenuAcceptSymbol(), MF_STR_CTRL_CHANGE_OPTION );
			break;
		case MF_CM_PROC:
			if( mfMenuIsPressed( mfMenuAcceptButton() ) ){
				(*((unsigned int *)var))++;
				if( ! ((char **)items)[*((unsigned int *)var)] ) *((unsigned int *)var) = 0;
			} else if( mfMenuIsPressed( PSP_CTRL_SQUARE ) ){
				if( ! *((unsigned int *)var) ){
					while( ((char **)items)[*((unsigned int *)var) + 1] ) (*((unsigned int *)var))++;
				} else{
					(*((unsigned int *)var))--;
				}
			}
			break;
	}
	return true;
}

bool mfCtrlDefCallback( MfMessage message, const char *label, void *func, void *arg, void *ex )
{
	switch( mfMenuMaskMainMessage( message ) ){
		case MF_CM_INIT:
		case MF_CM_TERM:
		case MF_CM_LABEL:
			break;
		case MF_CM_INFO:
			mfMenuSetInfoText( *((unsigned int *)ex) | MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(%s)%s", mfMenuAcceptSymbol(), MF_STR_CTRL_ENTER );
			break;
		case MF_CM_PROC:
			if( mfMenuIsPressed( mfMenuAcceptButton() ) ){
				mfMenuProc( (MfMenuProc)func );
			}
			break;
	}
	return true;
}

bool mfCtrlDefExtra( MfMessage message, const char *label, void *func, void *arg, void *ex )
{
	switch( mfMenuMaskMainMessage( message ) ){
		case MF_CM_INIT:
		case MF_CM_TERM:
		case MF_CM_LABEL:
			break;
		case MF_CM_INFO:
			mfMenuSetInfoText( *((unsigned int *)ex) | MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(%s)%s", mfMenuAcceptSymbol(), MF_STR_CTRL_EXECUTE );
			break;
		case MF_CM_PROC:
			if( mfMenuIsPressed( mfMenuAcceptButton() ) ){
				mfMenuSetExtra( (MfMenuExtraProc)func, arg );
			}
			break;
	}
	return true;
}

bool mfCtrlDefGetButtons( MfMessage message, const char *label, void *var, void *pref, void *ex )
{
	switch( mfMenuMaskMainMessage( message ) ){
		case MF_CM_INIT:
		case MF_CM_TERM:
		case MF_CM_LABEL:
			break;
		case MF_CM_INFO:
			mfMenuSetInfoText( *((unsigned int *)ex) | MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(%s)%s", mfMenuAcceptSymbol(), MF_STR_CTRL_ASSIGN );
			break;
		case MF_CM_PROC:
			if( mfMenuIsPressed( mfMenuAcceptButton() ) ){
				mfDialogDetectbuttons( label, ((MfCtrlDefGetButtonsPref *)pref)->availButtons, (PadutilButtons *)var );
			}
			break;
	}
	return true;
}

bool mfCtrlDefGetNumber( MfMessage message, const char *label, void *var, void *pref, void *ex )
{
	unsigned int value = mfCtrlVarGetNum( var, ((MfCtrlDefGetNumberPref *)pref)->max );
	
	switch( mfMenuMaskMainMessage( message ) ){
		case MF_CM_INIT:
		case MF_CM_TERM:
			break;
		case MF_CM_LABEL:
			if( pref && ((MfCtrlDefGetNumberPref *)pref)->unit ){
				mfCtrlSetLabel( ex, "%s: %d %s", label, value, ((MfCtrlDefGetNumberPref *)pref)->unit );
			} else{
				mfCtrlSetLabel( ex, "%s: %d", label, value );
			}
			break;
		case MF_CM_INFO:
			mfMenuSetInfoText( *((unsigned int *)ex) | MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(%s)%s, (%s)%s, (%s)%s", PB_SYM_PSP_SQUARE, MF_STR_CTRL_DECREASE, mfMenuAcceptSymbol(), MF_STR_CTRL_INCREASE, PB_SYM_PSP_TRIANGLE, MF_STR_CTRL_EDIT );
			break;
		case MF_CM_PROC:
			if( mfMenuIsPressed( PSP_CTRL_SQUARE ) ){
				if( pref && ((MfCtrlDefGetNumberPref *)pref)->width ){
					if( value >= ((MfCtrlDefGetNumberPref *)pref)->width ) value -= ((MfCtrlDefGetNumberPref *)pref)->width;
				} else if( value > ((MfCtrlDefGetNumberPref *)pref)->min ){
					value--;
				}
			} else if( mfMenuIsPressed( mfMenuAcceptButton() ) ){
				if( pref && ((MfCtrlDefGetNumberPref *)pref)->width ){
					if( ((MfCtrlDefGetNumberPref *)pref)->max - value >= ((MfCtrlDefGetNumberPref *)pref)->width ) value += ((MfCtrlDefGetNumberPref *)pref)->width;
				} else if( value < ((MfCtrlDefGetNumberPref *)pref)->max ){
					value++;
				}
			} else if( mfMenuIsPressed( PSP_CTRL_TRIANGLE ) ){
				mfDialogNumedit( label, ((MfCtrlDefGetNumberPref *)pref)->unit, var, ((MfCtrlDefGetNumberPref *)pref)->max );
			}
			break;
	}
	
	mfCtrlVarSetNum( var, ((MfCtrlDefGetNumberPref *)pref)->max, value );
	
	return true;
}

unsigned int mfCtrlVarGetNum( void *num, unsigned int max )
{
	if( max <= UINT8_MAX ){
		return *((uint8_t *)num);
	} else if( max <= UINT16_MAX ){
		return *((uint16_t *)num);
	} else{
		return *((uint32_t *)num);
	}
}

void mfCtrlVarSetNum( void *num, unsigned int max, unsigned int value )
{
	if( max <= UINT8_MAX ){
		*((uint8_t *)num)  = value;
	} else if( max <= UINT16_MAX ){
		*((uint16_t *)num) = value;
	} else{
		*((uint32_t *)num) = value;
	}
}

bool mfCtrlDefIniLoad( MfMessage message, const char *label, void *loader, void *pref, void *ex )
{
	switch( mfMenuMaskMainMessage( message ) ){
		case MF_CM_INIT:
		case MF_CM_TERM:
		case MF_CM_LABEL:
			break;
		case MF_CM_INFO:
			mfMenuSetInfoText( *((unsigned int *)ex) | MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(%s)%s", mfMenuAcceptSymbol(), MF_STR_CTRL_CHOOSEFILE );
			break;
		case MF_CM_PROC:
			if( mfMenuIsPressed( mfMenuAcceptButton() ) ){
				struct mf_ctrl_ini_params *params = mfMemoryTempAlloc( sizeof( struct mf_ctrl_ini_params ) );
				if( params ){
					params->proc   = loader;
					params->pref   = pref;
					params->action = MF_CTRL_INI_LOAD;
					params->path   = mfMemoryTempAlloc( params->pref->pathMax );
					if( params->path ){
						params->path[0] = '\0';
						mfMenuSetExtra( (MfMenuExtraProc)mf_ctrl_ini_handler, params );
					} else{
						mfMemoryFree( params );
						mf_ctrl_ini_error( MF_CTRL_INI_ERROR_NOT_ENOUGH_MEMORY );
						return false;
					}
				} else{
					mf_ctrl_ini_error( MF_CTRL_INI_ERROR_NOT_ENOUGH_MEMORY );
					return false;
				}
			}
			break;
	}
	return true;
}

bool mfCtrlDefIniSave( MfMessage message, const char *label, void *saver, void *pref, void *ex )
{
	switch( mfMenuMaskMainMessage( message ) ){
		case MF_CM_INIT:
		case MF_CM_TERM:
		case MF_CM_LABEL:
			break;
		case MF_CM_INFO:
			mfMenuSetInfoText( *((unsigned int *)ex) | MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(%s)%s", mfMenuAcceptSymbol(), MF_STR_CTRL_SAVEFILE );
			break;
		case MF_CM_PROC:
			if( mfMenuIsPressed( mfMenuAcceptButton() ) ){
				struct mf_ctrl_ini_params *params = mfMemoryTempAlloc( sizeof( struct mf_ctrl_ini_params ) );
				if( params ){
					params->proc   = saver;
					params->pref   = pref;
					params->action = MF_CTRL_INI_SAVE;
					params->path   = mfMemoryTempAlloc( params->pref->pathMax );
					if( params->path ){
						params->path[0] = '\0';
						mfMenuSetExtra( (MfMenuExtraProc)mf_ctrl_ini_handler, params );
					} else{
						mfMemoryFree( params );
						mf_ctrl_ini_error( MF_CTRL_INI_ERROR_NOT_ENOUGH_MEMORY );
						return false;
					}
				} else{
					mf_ctrl_ini_error( MF_CTRL_INI_ERROR_NOT_ENOUGH_MEMORY );
					return false;
				}
			}
			break;
	}
	return true;
}

static void mf_ctrl_ini_handler( MfMessage message, void *arg )
{
	struct mf_ctrl_ini_params *params = arg;
	
	if( message & MF_MM_INIT ){
		unsigned int options;
		
		if( params->action == MF_CTRL_INI_SAVE ){
			options = CDIALOG_GETFILENAME_SAVE | CDIALOG_GETFILENAME_OVERWRITEPROMPT;
		} else{
			options = CDIALOG_GETFILENAME_OPEN | CDIALOG_GETFILENAME_FILEMUSTEXIST;
		}
		
		if( ! mfDialogGetfilename( params->pref->title, params->pref->initDir, params->pref->initFilename, params->path, params->pref->pathMax, options ) ){
			mfMenuSendSignal( MF_CTRL_DEF_INI_SIG_FINISH );
		}
	} else if( message & MF_MM_PROC ){
		 if( message & MF_CTRL_DEF_INI_SIG_ACCEPT ){
			int ret = ( params->proc )( params->action, params->path );
			if( ret != MF_CTRL_INI_ERROR_OK ) mf_ctrl_ini_error( ret );
			mfMenuSendSignal( MF_CTRL_DEF_INI_SIG_FINISH );
		} else if( message & MF_CTRL_DEF_INI_SIG_FINISH ){
			mfMemoryFree( params->path );
			mfMemoryFree( params );
			mfMenuExitExtra();
		} else if( message & MF_DM_FINISH ){
			if( ! mfDialogLastResult() ){
				mfMenuSendSignal( MF_CTRL_DEF_INI_SIG_FINISH );
			} else{
				mfMenuSetInfoText( MF_MENU_INFOTEXT_SET_MIDDLE_LINE, params->action == MF_CTRL_INI_SAVE ? MF_STR_SAVING : MF_STR_LOADING, params->path );
				mfMenuSendSignal( MF_CTRL_DEF_INI_SIG_ACCEPT );
			}
		}
	}
}

static void mf_ctrl_ini_error( int error )
{
	switch( error ){
		case MF_CTRL_INI_ERROR_OK: break;
		case MF_CTRL_INI_ERROR_INVALID_CONF:
			mfDialogMessage( MF_STR_ERROR, MF_STR_ERROR_INVALID_CONF, 0, false );
			break;
		case MF_CTRL_INI_ERROR_UNSUPPORTED_VERSION:
			mfDialogMessage( MF_STR_ERROR, MF_STR_ERROR_UNSUPPORTED_VERSION, 0, false );
			break;
		case MF_CTRL_INI_ERROR_NOT_ENOUGH_MEMORY:
			if( ! mfDialogMessage( MF_STR_ERROR, MF_STR_ERROR_INICTRL MF_STR_ERROR_NOT_ENOUGH_MEMORY, CG_ERROR_NOT_ENOUGH_MEMORY, false ) ){
				/* メモリ不足でメッセージダイアログ表示も出来ない場合は直接表示 */
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "%s: %s", MF_STR_ERROR, MF_STR_ERROR_NOT_ENOUGH_MEMORY );
				mfMenuWait( MF_ERROR_DELAY );
			}
			break;
		default:
			mfDialogMessage( MF_STR_ERROR, MF_STR_ERROR_INICTRL, error, false );
	}
}

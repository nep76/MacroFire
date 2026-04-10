/*=========================================================

	mfctrl.c

	組み込みコントロール。

=========================================================*/
#include "mfctrl.h"

void mfCtrlSetLabel( void *arg, char *format, ... )
{
	va_list ap;
	
	va_start( ap, format );
	vsnprintf( (char *)arg, MF_CTRL_BUFFER_LENGTH, format, ap );
	va_end( ap );
}

bool mfCtrlDefBool( MfMessage message, const char *label, void *var, void *unused, void *ex )
{
	switch( message ){
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
	switch( message ){
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
	switch( message ){
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
	switch( message ){
		case MF_CM_INIT:
		case MF_CM_TERM:
		case MF_CM_LABEL:
			break;
		case MF_CM_INFO:
			mfMenuSetInfoText( *((unsigned int *)ex) | MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(%s)%s", mfMenuAcceptSymbol(), MF_STR_CTRL_EXECUTE );
			break;
		case MF_CM_PROC:
			if( mfMenuIsPressed( mfMenuAcceptButton() ) ){
				mfMenuSetExtra( (MfMenuProc)func );
			}
			break;
	}
	return true;
}

bool mfCtrlDefGetButtons( MfMessage message, const char *label, void *var, void *pref, void *ex )
{
	switch( message ){
		case MF_CM_INIT:
		case MF_CM_TERM:
		case MF_CM_LABEL:
			break;
		case MF_CM_INFO:
			mfMenuSetInfoText( *((unsigned int *)ex) | MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(%s)%s", mfMenuAcceptSymbol(), MF_STR_CTRL_ASSIGN );
			break;
		case MF_CM_PROC:
			if( mfMenuIsPressed( mfMenuAcceptButton() ) ){
				mfDialogDetectbuttonsInit( label, ((MfCtrlDefGetButtonsPref *)pref)->availButtons, (PadutilButtons *)var );
			}
			break;
	}
	return true;
}

bool mfCtrlDefGetNumber( MfMessage message, const char *label, void *var, void *pref, void *ex )
{
	unsigned int value = mfCtrlVarGetNum( var, ((MfCtrlDefGetNumberPref *)pref)->max );
	
	switch( message ){
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
				mfDialogNumeditInit( label, ((MfCtrlDefGetNumberPref *)pref)->unit, var, ((MfCtrlDefGetNumberPref *)pref)->max );
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
/*
bool mfCtrlDefGetFilename( MfMessage message, const char *label, void *var, void *pref, void *ex )
{
	switch( message ){
		case MF_CM_INIT:
		case MF_CM_TERM:
		case MF_CM_LABEL:
			break;
		case MF_CM_INFO:
			mfCtrlSetInfo( ex, MF_CTRL_INFO_LOWER_LINE, "\x85 = Select file" );
			break;
		case MF_CM_PROC:
			if( mfMenuIsPressed( mfMenuAcceptButton() ) ){
				mfDialogGetfilenameInit(
					label,
					((MfCtrlDefGetFilenamePref *)pref)->initDir,
					((MfCtrlDefGetFilenamePref *)pref)->initFilename,
					var,
					((MfCtrlDefGetFilenamePref *)pref)->pathMax,
					((MfCtrlDefGetFilenamePref *)pref)->options
				);
			}
			break;
	}
	
	return true;
}
*/

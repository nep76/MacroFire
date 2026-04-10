/*=========================================================
	
	lang_en.h
	
	MacroFireì‡Ç≈égópÇ∑ÇÈï∂éöóÒíËêîÅB
	
=========================================================*/
#ifndef CDIALOG_STRINGS
#define CDIALOG_STRINGS

#define CDIALOG_STR_HELP_LABEL "Help"

#define CDIALOG_STR_MESSAGE_YES "Yes"
#define CDIALOG_STR_MESSAGE_NO  "No"
#define CDIALOG_STR_MESSAGE_OK  "OK"

#define CDIALOG_STR_DETECTBUTTONS_DESC            "Please choose one."
#define CDIALOG_STR_DETECTBUTTONS_START           "  " PB_SYM_PSP_CIRCLE "  : Starting to detect"
#define CDIALOG_STR_DETECTBUTTONS_CLEAR           "  " PB_SYM_PSP_SQUARE "  : Clear current"
#define CDIALOG_STR_DETECTBUTTONS_CANCEL          "  " PB_SYM_PSP_CROSS  "  : Cancel"
#define CDIALOG_STR_DETECTBUTTONS_ACCEPT          "START: Accept"
#define CDIALOG_STR_DETECTBUTTONS_CURRENT_BUTTONS "Current buttons:"
#define CDIALOG_STR_DETECTBUTTONS_DETECT_DESC     "Please press the button combination.\n\nWhen you want to exit the dialog,\npress the already pressed button."
#define CDIALOG_STR_DETECTBUTTONS_NOW_DETECTING   "Now detecting..."

#define CDIALOG_STR_GETFILENAME_SAVE_IN                 "Save in"
#define CDIALOG_STR_GETFILENAME_LOOK_IN                 "Look in"
#define CDIALOG_STR_GETFILENAME_FILENAME                "Filename:"
#define CDIALOG_STR_GETFILENAME_CONFIRM_CREATE_LABEL    "Creation"
#define CDIALOG_STR_GETFILENAME_CONFIRM_CREATE          "File does not exist.\nDo you want to create it?"
#define CDIALOG_STR_GETFILENAME_CONFIRM_OVERWRITE_LABEL "Overwrite"
#define CDIALOG_STR_GETFILENAME_CONFIRM_OVERWRITE       "File already exist.\nAre you sure you want to overwrite it?"
#define CDIALOG_STR_GETFILENAME_ERROR_INVALID_FILENAME  "Invalid filename"
#define CDIALOG_STR_GETFILENAME_ERROR_ILLEGAL_CHAR      "Filename contains illegal characters\n\nIllegal characters: \\/:?\"<>|"
#define CDIALOG_STR_GETFILENAME_ERROR_EMPTY             "Filename is empty"
#define CDIALOG_STR_GETFILENAME_ERROR_SAME_AS_DIR_NAME  "This filename already exist as directory name in the current directory."
#define CDIALOG_STR_GETFILENAME_ERROR_NOT_FOUND         "File does not exist."
#define CDIALOG_STR_GETFILENAME_INPUT                   "Input filename"
#define CDIALOG_STR_GETFILENAME_EXTRA_MENU_LABEL        "Extra menu"
#define CDIALOG_STR_GETFILENAME_EXTRA_MENU              "Sorry. Not implemented yet."
#define CDIALOG_STR_GETFILENAME_HELP \
	"L/R   = MoveFocus\n" \
	"START = Accept\n" \
	PB_SYM_PSP_CROSS "     = Cancel\n\n" \
	"*Filelist\n\n" \
	"  " PB_SYM_PSP_UP   PB_SYM_PSP_DOWN  "     = Move\n" \
	"  " PB_SYM_PSP_LEFT PB_SYM_PSP_RIGHT "     = MovePage\n" \
	"  " PB_SYM_PSP_CIRCLE               "      = Enter\n" \
	"  " PB_SYM_PSP_TRIANGLE             "      = Parent Directory\n" \
	"  SELECT = " CDIALOG_STR_GETFILENAME_EXTRA_MENU_LABEL " (Not implemented yet)\n\n" \
	"*Filename\n\n" \
	"  " PB_SYM_PSP_CIRCLE " = Input"

#define CDIALOG_STR_NUMEDIT_HELP \
	PB_SYM_PSP_LEFT PB_SYM_PSP_RIGHT " = Move\n" \
	PB_SYM_PSP_UP   PB_SYM_PSP_DOWN  " = Change value\n\n" \
	PB_SYM_PSP_CIRCLE " = Accept\n" \
	PB_SYM_PSP_CROSS  " = Cancel"

#define CDIALOG_STR_SOSK_CANCEL_LABEL "Cancellation"
#define CDIALOG_STR_SOSK_CANCEL       "Are you sure you want to cancel?"
#define CDIALOG_STR_SOSK_HELP \
	PB_SYM_PSP_UP PB_SYM_PSP_RIGHT PB_SYM_PSP_DOWN PB_SYM_PSP_LEFT " = Move\n" \
	"L/R  = MoveCursor\n\n" \
	PB_SYM_PSP_CIRCLE   " = Input\n" \
	PB_SYM_PSP_TRIANGLE " = Space\n" \
	PB_SYM_PSP_SQUARE   " = Backspace\n\n" \
	"SELECT = Change layout\n\n" \
	"START = Accept\n" \
	PB_SYM_PSP_CROSS "     = Cancel"

#endif

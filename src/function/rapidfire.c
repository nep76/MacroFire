/*
	RapidFire
*/

#include "rapidfire.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static char *rapidire_mode_to_string( int mode );
static int rapidfire_string_to_mode( char *mode );
static MfMenuRc rapidfire_load( void );
static MfMenuRc rapidfire_save( void );

/*-----------------------------------------------
	ローカル変数と定数
-----------------------------------------------*/
static RapidfireConf st_rfconf[] = {
	{ PSP_CTRL_CIRCLE,   0 },
	{ PSP_CTRL_CROSS,    0 },
	{ PSP_CTRL_SQUARE,   0 },
	{ PSP_CTRL_TRIANGLE, 0 },
	{ PSP_CTRL_UP,       0 },
	{ PSP_CTRL_RIGHT,    0 },
	{ PSP_CTRL_DOWN,     0 },
	{ PSP_CTRL_LEFT,     0 },
	{ PSP_CTRL_LTRIGGER, 0 },
	{ PSP_CTRL_RTRIGGER, 0 },
	{ PSP_CTRL_START,    0 },
	{ PSP_CTRL_SELECT,   0 }
};
static MfMenuCallback st_callback;

static short st_rapid        = 0;
static int st_release_delay  = RAPIDFIRE_DEFAULT_RELEASE_DELAY;
static int st_press_delay    = RAPIDFIRE_DEFAULT_PRESS_DELAY;
static int st_wait_cnt       = 0;

static MfMenuItem st_menu_table[] = {
	{ MT_OPTION, "Circle  ", 0, &(st_rfconf[0].mode),  { { .string = "NORMAL" }, { .string = "RAPID" }, { .string = "AUTO-RAPID"}, { .string = "AUTO-HOLD" }, { .string = NULL } } },
	{ MT_OPTION, "Cross   ", 0, &(st_rfconf[1].mode),  { { .string = "NORMAL" }, { .string = "RAPID" }, { .string = "AUTO-RAPID"}, { .string = "AUTO-HOLD" }, { .string = NULL } } },
	{ MT_OPTION, "Square  ", 0, &(st_rfconf[2].mode),  { { .string = "NORMAL" }, { .string = "RAPID" }, { .string = "AUTO-RAPID"}, { .string = "AUTO-HOLD" }, { .string = NULL } } },
	{ MT_OPTION, "Triangle", 0, &(st_rfconf[3].mode),  { { .string = "NORMAL" }, { .string = "RAPID" }, { .string = "AUTO-RAPID"}, { .string = "AUTO-HOLD" }, { .string = NULL } } },
	{ MT_NULL },
	{ MT_OPTION, "Up      ", 0, &(st_rfconf[4].mode),  { { .string = "NORMAL" }, { .string = "RAPID" }, { .string = "AUTO-RAPID"}, { .string = "AUTO-HOLD" }, { .string = NULL } } },
	{ MT_OPTION, "Right   ", 0, &(st_rfconf[5].mode),  { { .string = "NORMAL" }, { .string = "RAPID" }, { .string = "AUTO-RAPID"}, { .string = "AUTO-HOLD" }, { .string = NULL } } },
	{ MT_OPTION, "Down    ", 0, &(st_rfconf[6].mode),  { { .string = "NORMAL" }, { .string = "RAPID" }, { .string = "AUTO-RAPID"}, { .string = "AUTO-HOLD" }, { .string = NULL } } },
	{ MT_OPTION, "Left    ", 0, &(st_rfconf[7].mode),  { { .string = "NORMAL" }, { .string = "RAPID" }, { .string = "AUTO-RAPID"}, { .string = "AUTO-HOLD" }, { .string = NULL } } },
	{ MT_NULL },
	{ MT_OPTION, "L-trig  ", 0, &(st_rfconf[8].mode),  { { .string = "NORMAL" }, { .string = "RAPID" }, { .string = "AUTO-RAPID"}, { .string = "AUTO-HOLD" }, { .string = NULL } } },
	{ MT_OPTION, "R-trig  ", 0, &(st_rfconf[9].mode),  { { .string = "NORMAL" }, { .string = "RAPID" }, { .string = "AUTO-RAPID"}, { .string = "AUTO-HOLD" }, { .string = NULL } } },
	{ MT_NULL },
	{ MT_OPTION, "START   ", 0, &(st_rfconf[10].mode), { { .string = "NORMAL" }, { .string = "RAPID" }, { .string = "AUTO-RAPID"}, { .string = "AUTO-HOLD" }, { .string = NULL } } },
	{ MT_OPTION, "SELECT  ", 0, &(st_rfconf[11].mode), { { .string = "NORMAL" }, { .string = "RAPID" }, { .string = "AUTO-RAPID"}, { .string = "AUTO-HOLD" }, { .string = NULL } } },
	
	{ MT_NULL },
	{ MT_GET_NUMBERS, "Release delay", 0, &(st_release_delay), { { .string = "Set release delay" }, { .string = "times" }, { .integer = 2 } } },
	{ MT_GET_NUMBERS, "Press delay  ", 0, &(st_press_delay),   { { .string = "Set press delay"   }, { .string = "times" }, { .integer = 2 } } },
	
	{ MT_NULL },
	{ MT_CALLBACK, "Load from MemoryStick", 0, &(st_callback), { { .pointer = rapidfire_load }, { .pointer = NULL } } },
	{ MT_CALLBACK, "Save to MemoryStick",   0, &(st_callback), { { .pointer = rapidfire_save }, { .pointer = NULL } } }
};

/*=============================================*/

static char *rapidire_mode_to_string( int mode )
{
	switch( mode ){
		case RAPIDFIRE_MODE_SEMI_RAPID:
			return RAPIDFIRE_DID_MODESEMIRAPID;
		case RAPIDFIRE_MODE_AUTO_RAPID:
			return RAPIDFIRE_DID_MODEAUTORAPID;
		case RAPIDFIRE_MODE_AUTO_HOLD:
			return RAPIDFIRE_DID_MODEAUTOHOLD;
		default:
			return RAPIDFIRE_DID_MODENORMAL;
	}
}

static int rapidfire_string_to_mode( char *mode )
{
	if( strcmp( mode, RAPIDFIRE_DID_MODESEMIRAPID ) == 0 ){
		return RAPIDFIRE_MODE_SEMI_RAPID;
	} else if( strcmp( mode, RAPIDFIRE_DID_MODEAUTORAPID ) == 0 ){
		return RAPIDFIRE_MODE_AUTO_RAPID;
	} else if( strcmp( mode, RAPIDFIRE_DID_MODEAUTOHOLD ) == 0 ){
		return RAPIDFIRE_MODE_AUTO_HOLD;
	} else{
		return RAPIDFIRE_MODE_NORMAL;
	}
}

static MfMenuRc rapidfire_load( void )
{
	if( ! mfMenuGetFilenameIsReady() ){
		MfMenuRc rc = mfMenuGetFilenameInit(
			"Open rapidfire preference",
			CGF_OPEN | CGF_FILEMUSTEXIST,
			"ms0:",
			(char *)memsceMalloc( 128 ),
			128,
			(char *)memsceMalloc( 128 ),
			128
		);
		if( rc != MR_ENTER ) return MR_BACK;
		
		return MR_CONTINUE;
	} else{
		IniUID ini;
		char *inipath, *path, *name;
		MfMenuRc rc = mfMenuGetFilename( &path, &name );
		
		if( rc != MR_ENTER ) return rc;
		
		inipath = (char *)memsceMalloc( strlen( path ) + strlen( name ) + 2 );
		if( ! inipath ){
			memsceFree( path );
			memsceFree( name );
			return MR_BACK;
		} else{
			sprintf( inipath, "%s/%s", path, name );
			memsceFree( path );
			memsceFree( name );
		}
		
		ini = inimgrNew();
		if( ini > 0 ){
			if( inimgrLoad( ini, inipath ) == 0 ){
				char mode[16];
				int version = inimgrGetInt( ini, "default", RAPIDFIRE_DATA_SIGNATURE, 0 );
				if( version <= 1 ){
					blitString( blitOffsetChar( 3 ), blitOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FCCOLOR, "Initialization file is invalid or too lower version." );
					mfMenuWait( RAPIDFIRE_ERROR_DISPLAY_MICROSEC );
				} else{
					inimgrGetString( ini, "Mode", RAPIDFIRE_DID_CIRCLE, RAPIDFIRE_DID_MODENORMAL, mode, sizeof( mode ) );
					st_rfconf[RAPIDFIRE_BUTTON_CIRCLE].mode = rapidfire_string_to_mode( mode );
					inimgrGetString( ini, "Mode", RAPIDFIRE_DID_CROSS, RAPIDFIRE_DID_MODENORMAL, mode, sizeof( mode ) );
					st_rfconf[RAPIDFIRE_BUTTON_CROSS].mode = rapidfire_string_to_mode( mode );
					inimgrGetString( ini, "Mode", RAPIDFIRE_DID_SQUARE, RAPIDFIRE_DID_MODENORMAL, mode, sizeof( mode ) );
					st_rfconf[RAPIDFIRE_BUTTON_SQUARE].mode = rapidfire_string_to_mode( mode );
					inimgrGetString( ini, "Mode", RAPIDFIRE_DID_TRIANGLE, RAPIDFIRE_DID_MODENORMAL, mode, sizeof( mode ) );
					st_rfconf[RAPIDFIRE_BUTTON_TRIANGLE].mode = rapidfire_string_to_mode( mode );
					inimgrGetString( ini, "Mode", RAPIDFIRE_DID_UP, RAPIDFIRE_DID_MODENORMAL, mode, sizeof( mode ) );
					st_rfconf[RAPIDFIRE_BUTTON_UP].mode = rapidfire_string_to_mode( mode );
					inimgrGetString( ini, "Mode", RAPIDFIRE_DID_RIGHT, RAPIDFIRE_DID_MODENORMAL, mode, sizeof( mode ) );
					st_rfconf[RAPIDFIRE_BUTTON_RIGHT].mode = rapidfire_string_to_mode( mode );
					inimgrGetString( ini, "Mode", RAPIDFIRE_DID_DOWN, RAPIDFIRE_DID_MODENORMAL, mode, sizeof( mode ) );
					st_rfconf[RAPIDFIRE_BUTTON_DOWN].mode = rapidfire_string_to_mode( mode );
					inimgrGetString( ini, "Mode", RAPIDFIRE_DID_LEFT, RAPIDFIRE_DID_MODENORMAL, mode, sizeof( mode ) );
					st_rfconf[RAPIDFIRE_BUTTON_LEFT].mode = rapidfire_string_to_mode( mode );
					inimgrGetString( ini, "Mode", RAPIDFIRE_DID_LTRIGGER, RAPIDFIRE_DID_MODENORMAL, mode, sizeof( mode ) );
					st_rfconf[RAPIDFIRE_BUTTON_LTRIGGER].mode = rapidfire_string_to_mode( mode );
					inimgrGetString( ini, "Mode", RAPIDFIRE_DID_RTRIGGER, RAPIDFIRE_DID_MODENORMAL, mode, sizeof( mode ) );
					st_rfconf[RAPIDFIRE_BUTTON_RTRIGGER].mode = rapidfire_string_to_mode( mode );
					inimgrGetString( ini, "Mode", RAPIDFIRE_DID_START, RAPIDFIRE_DID_MODENORMAL, mode, sizeof( mode ) );
					st_rfconf[RAPIDFIRE_BUTTON_START].mode = rapidfire_string_to_mode( mode );
					inimgrGetString( ini, "Mode", RAPIDFIRE_DID_SELECT, RAPIDFIRE_DID_MODENORMAL, mode, sizeof( mode ) );
					st_rfconf[RAPIDFIRE_BUTTON_SELECT].mode = rapidfire_string_to_mode( mode );
					
					st_release_delay = inimgrGetInt( ini, "Delay", RAPIDFIRE_DID_RDELAY, RAPIDFIRE_DEFAULT_RELEASE_DELAY );
					st_press_delay   = inimgrGetInt( ini, "Delay", RAPIDFIRE_DID_PDELAY, RAPIDFIRE_DEFAULT_PRESS_DELAY );
					
					blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FGCOLOR, "Loaded from %s.", inipath );
					mfMenuWait( RAPIDFIRE_INFO_DISPLAY_MICROSEC );
				}
			}
			inimgrDestroy( ini );
		}
		
		memsceFree( inipath );
		
		return MR_BACK;
	}
}

static MfMenuRc rapidfire_save( void )
{
	if( ! mfMenuGetFilenameIsReady() ){
		MfMenuRc rc = mfMenuGetFilenameInit(
			"Save rapidfire preference",
			CGF_SAVE | CGF_OVERWRITEPROMPT,
			"ms0:",
			(char *)memsceMalloc( 128 ),
			128,
			(char *)memsceMalloc( 128 ),
			128
		);
		if( rc != MR_ENTER ) return MR_BACK;
		
		return MR_CONTINUE;
	} else{
		IniUID ini;
		char *inipath, *path, *name;
		MfMenuRc rc = mfMenuGetFilename( &path, &name );
		
		if( rc != MR_ENTER ) return rc;
		
		inipath = (char *)memsceMalloc( strlen( path ) + strlen( name ) + 2 );
		if( ! inipath ){
			memsceFree( path );
			memsceFree( name );
			return MR_BACK;
		} else{
			sprintf( inipath, "%s/%s", path, name );
			memsceFree( path );
			memsceFree( name );
		}
		
		ini = inimgrNew();
		if( ini > 0 ){
			inimgrSetInt( ini, "default", RAPIDFIRE_DATA_SIGNATURE, RAPIDFIRE_DATA_VERSION );
			inimgrSetString( ini, "Mode",  RAPIDFIRE_DID_CIRCLE,   rapidire_mode_to_string( st_rfconf[RAPIDFIRE_BUTTON_CIRCLE].mode ) );
			inimgrSetString( ini, "Mode",  RAPIDFIRE_DID_CROSS,    rapidire_mode_to_string( st_rfconf[RAPIDFIRE_BUTTON_CROSS].mode ) );
			inimgrSetString( ini, "Mode",  RAPIDFIRE_DID_SQUARE,   rapidire_mode_to_string( st_rfconf[RAPIDFIRE_BUTTON_SQUARE].mode ) );
			inimgrSetString( ini, "Mode",  RAPIDFIRE_DID_TRIANGLE, rapidire_mode_to_string( st_rfconf[RAPIDFIRE_BUTTON_TRIANGLE].mode ) );
			inimgrSetString( ini, "Mode",  RAPIDFIRE_DID_UP,       rapidire_mode_to_string( st_rfconf[RAPIDFIRE_BUTTON_UP].mode ) );
			inimgrSetString( ini, "Mode",  RAPIDFIRE_DID_RIGHT,    rapidire_mode_to_string( st_rfconf[RAPIDFIRE_BUTTON_RIGHT].mode ) );
			inimgrSetString( ini, "Mode",  RAPIDFIRE_DID_DOWN,     rapidire_mode_to_string( st_rfconf[RAPIDFIRE_BUTTON_DOWN].mode ) );
			inimgrSetString( ini, "Mode",  RAPIDFIRE_DID_LEFT,     rapidire_mode_to_string( st_rfconf[RAPIDFIRE_BUTTON_LEFT].mode ) );
			inimgrSetString( ini, "Mode",  RAPIDFIRE_DID_LTRIGGER, rapidire_mode_to_string( st_rfconf[RAPIDFIRE_BUTTON_LTRIGGER].mode ) );
			inimgrSetString( ini, "Mode",  RAPIDFIRE_DID_RTRIGGER, rapidire_mode_to_string( st_rfconf[RAPIDFIRE_BUTTON_RTRIGGER].mode ) );
			inimgrSetString( ini, "Mode",  RAPIDFIRE_DID_START,    rapidire_mode_to_string( st_rfconf[RAPIDFIRE_BUTTON_START].mode ) );
			inimgrSetString( ini, "Mode",  RAPIDFIRE_DID_SELECT,   rapidire_mode_to_string( st_rfconf[RAPIDFIRE_BUTTON_SELECT].mode ) );
			inimgrSetInt( ini, "Delay", RAPIDFIRE_DID_RDELAY,   st_release_delay );
			inimgrSetInt( ini, "Delay", RAPIDFIRE_DID_PDELAY,   st_press_delay );
			inimgrSave( ini, inipath );
			inimgrDestroy( ini );
			
			blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FGCOLOR, "Saved to %s.", inipath );
			mfMenuWait( RAPIDFIRE_INFO_DISPLAY_MICROSEC );
		}
		
		memsceFree( inipath );
		
		return MR_BACK;
	}
}

void rapidfireInit( void )
{
	st_wait_cnt = 0;
}

void rapidfireMain( MfCallMode mode, SceCtrlData *pad_data, void *argp )
{
	int i;
	
	if( st_wait_cnt && mode == MF_CALL_READ ) st_wait_cnt--;
	
	for( i = 0; i < MF_ARRAY_NUM( st_rfconf ); i++ ){
		if(
			(pad_data->Buttons & st_rfconf[i].button && st_rfconf[i].mode == RAPIDFIRE_MODE_SEMI_RAPID ) ||
			st_rfconf[i].mode == RAPIDFIRE_MODE_AUTO_RAPID
		 ){
			pad_data->Buttons ^= st_rapid ? st_rfconf[i].button : 0;
		} else if( st_rfconf[i].mode == RAPIDFIRE_MODE_AUTO_HOLD ){
			pad_data->Buttons |= st_rfconf[i].button;
		}
	}
	
	if( ! st_wait_cnt && mode == MF_CALL_READ ){
		st_rapid = ( ! st_rapid );
		if( st_rapid ){
			st_wait_cnt = st_press_delay;
		} else{
			st_wait_cnt = st_release_delay;
		}
	}
	
	return;
}

MfMenuRc rapidfireMenu( SceCtrlData *pad_data, void *argp )
{
	static int selected    = 0;
	
	if( ! st_callback.func ){
		blitString( blitOffsetChar(  3 ), blitOffsetLine(  4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Please choose a rapidfire mode per buttons." );
		blitString( blitOffsetChar( 33 ), blitOffsetLine( 22 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "NORMAL    : standard control mode." );
		blitString( blitOffsetChar( 33 ), blitOffsetLine( 23 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "RAPID     : hold the button to rapidfire." );
		blitString( blitOffsetChar( 33 ), blitOffsetLine( 24 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "AUTO-RAPID: always to rapidfire." );
		blitString( blitOffsetChar( 33 ), blitOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "AUTO-HOLD : always to press and hold." );
		
		/* 目立つように警告はSave/Loadにカーソルがのった時のみ表示 */
		if( selected == RAPIDFIRE_LOAD || selected == RAPIDFIRE_SAVE ){
			blitString( blitOffsetChar( 5 ), blitOffsetLine( 28 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "IMPORTANT CAUTION\n  Verify that MemoryStick access indicator is not blinking.\n  Otherwise, will CRASH the currently running game!" );
		}
		
		switch( mfMenuUniDraw( blitOffsetChar( 5 ), blitOffsetLine( 6 ), st_menu_table, MF_ARRAY_NUM( st_menu_table ), &selected, 0 ) ){
			case MR_CONTINUE:
				break;
			case MR_ENTER:
				break;
			case MR_BACK:
				//selected = 0;
				return MR_BACK;
		}
	} else{
		MfMenuRc rc = ((RapidfireConfIo)(st_callback.func))();
		if( rc == MR_BACK ){
			st_callback.func = NULL;
		}
	}
	
	return MR_CONTINUE;
}

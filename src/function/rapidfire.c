/*
	RapidFire
*/

#include "rapidfire.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static char *rapidfire_mode_to_string( int mode );
static int rapidfire_string_to_mode( char *mode );
static MfMenuRc rapidfire_load( void );
static MfMenuRc rapidfire_save( void );
static bool rapidfire_ini_verck( IniUID ini );
static void rapidfire_ini_load( IniUID ini );

/*-----------------------------------------------
	ローカル変数と定数
-----------------------------------------------*/
static RapidfireConf st_rfconf[] = {
	{ RAPIDFIRE_DID_CIRCLE,   PSP_CTRL_CIRCLE,   0 },
	{ RAPIDFIRE_DID_CROSS,    PSP_CTRL_CROSS,    0 },
	{ RAPIDFIRE_DID_SQUARE,   PSP_CTRL_SQUARE,   0 },
	{ RAPIDFIRE_DID_TRIANGLE, PSP_CTRL_TRIANGLE, 0 },
	{ RAPIDFIRE_DID_UP,       PSP_CTRL_UP,       0 },
	{ RAPIDFIRE_DID_RIGHT,    PSP_CTRL_RIGHT,    0 },
	{ RAPIDFIRE_DID_DOWN,     PSP_CTRL_DOWN,     0 },
	{ RAPIDFIRE_DID_LEFT,     PSP_CTRL_LEFT,     0 },
	{ RAPIDFIRE_DID_LTRIGGER, PSP_CTRL_LTRIGGER, 0 },
	{ RAPIDFIRE_DID_RTRIGGER, PSP_CTRL_RTRIGGER, 0 },
	{ RAPIDFIRE_DID_START,    PSP_CTRL_START,    0 },
	{ RAPIDFIRE_DID_SELECT,   PSP_CTRL_SELECT,   0 }
};
static MfMenuCallback st_callback;

static int st_release_delay  = RAPIDFIRE_DEFAULT_RELEASE_DELAY;
static int st_press_delay    = RAPIDFIRE_DEFAULT_PRESS_DELAY;

char *st_options[] = {
	RAPIDFIRE_DID_MODENORMAL,
	RAPIDFIRE_DID_MODERAPID,
	RAPIDFIRE_DID_MODEAUTORAPID,
	RAPIDFIRE_DID_MODEHOLD,
	RAPIDFIRE_DID_MODEAUTOHOLD
};

static MfMenuItem st_menu_table[] = {
	{ "Circle  : %s", mfMenuDefOptionProc, &(st_rfconf[0].mode),  { { .pointer = &st_options }, { .integer = MF_ARRAY_NUM( st_options ) } } },
	{ "Cross   : %s", mfMenuDefOptionProc, &(st_rfconf[1].mode),  { { .pointer = &st_options }, { .integer = MF_ARRAY_NUM( st_options ) } } },
	{ "Square  : %s", mfMenuDefOptionProc, &(st_rfconf[2].mode),  { { .pointer = &st_options }, { .integer = MF_ARRAY_NUM( st_options ) } } },
	{ "Triangle: %s", mfMenuDefOptionProc, &(st_rfconf[3].mode),  { { .pointer = &st_options }, { .integer = MF_ARRAY_NUM( st_options ) } } },
	{ NULL },
	{ "Up      : %s", mfMenuDefOptionProc, &(st_rfconf[4].mode),  { { .pointer = &st_options }, { .integer = MF_ARRAY_NUM( st_options ) } } },
	{ "Right   : %s", mfMenuDefOptionProc, &(st_rfconf[5].mode),  { { .pointer = &st_options }, { .integer = MF_ARRAY_NUM( st_options ) } } },
	{ "Down    : %s", mfMenuDefOptionProc, &(st_rfconf[6].mode),  { { .pointer = &st_options }, { .integer = MF_ARRAY_NUM( st_options ) } } },
	{ "Left    : %s", mfMenuDefOptionProc, &(st_rfconf[7].mode),  { { .pointer = &st_options }, { .integer = MF_ARRAY_NUM( st_options ) } } },
	{ NULL },
	{ "L-trig  : %s", mfMenuDefOptionProc, &(st_rfconf[8].mode),  { { .pointer = &st_options }, { .integer = MF_ARRAY_NUM( st_options ) } } },
	{ "R-trig  : %s", mfMenuDefOptionProc, &(st_rfconf[9].mode),  { { .pointer = &st_options }, { .integer = MF_ARRAY_NUM( st_options ) } } },
	{ NULL },
	{ "START   : %s", mfMenuDefOptionProc, &(st_rfconf[10].mode), { { .pointer = &st_options }, { .integer = MF_ARRAY_NUM( st_options ) } } },
	{ "SELECT  : %s", mfMenuDefOptionProc, &(st_rfconf[11].mode), { { .pointer = &st_options }, { .integer = MF_ARRAY_NUM( st_options ) } } },
	{ NULL },
	{ "Release delay: %d %s", mfMenuDefGetNumberProc, &st_release_delay, { { .string = "Set release delay" }, { .string = "ms" }, { .integer = 4 }, { .integer = 18 }, { .integer = 9999 } } },
	{ "Press delay  : %d %s", mfMenuDefGetNumberProc, &st_press_delay  , { { .string = "Set press delay"   }, { .string = "ms" }, { .integer = 4 }, { .integer = 18 }, { .integer = 9999 } } },
	{ NULL },
	{ "Load from MemoryStick", mfMenuDefCallbackProc, &st_callback, { { .pointer = rapidfire_load }, { .pointer = NULL } } },
	{ "Save to MemoryStick",   mfMenuDefCallbackProc, &st_callback, { { .pointer = rapidfire_save }, { .pointer = NULL } } }
};

/*=============================================*/

static char *rapidfire_mode_to_string( int mode )
{
	switch( mode ){
		case RAPIDFIRE_MODE_SEMI_RAPID:
			return RAPIDFIRE_DID_MODERAPID;
		case RAPIDFIRE_MODE_AUTO_RAPID:
			return RAPIDFIRE_DID_MODEAUTORAPID;
		case RAPIDFIRE_MODE_HOLD:
			return RAPIDFIRE_DID_MODEHOLD;
		case RAPIDFIRE_MODE_AUTO_HOLD:
			return RAPIDFIRE_DID_MODEAUTOHOLD;
		default:
			return RAPIDFIRE_DID_MODENORMAL;
	}
}

static int rapidfire_string_to_mode( char *mode )
{
	if( strcmp( mode, RAPIDFIRE_DID_MODERAPID ) == 0 ){
		return RAPIDFIRE_MODE_SEMI_RAPID;
	} else if( strcmp( mode, RAPIDFIRE_DID_MODEAUTORAPID ) == 0 ){
		return RAPIDFIRE_MODE_AUTO_RAPID;
	} else if( strcmp( mode, RAPIDFIRE_DID_MODEHOLD ) == 0 ){
		return RAPIDFIRE_MODE_HOLD;
	} else if( strcmp( mode, RAPIDFIRE_DID_MODEAUTOHOLD ) == 0 ){
		return RAPIDFIRE_MODE_AUTO_HOLD;
	} else{
		return RAPIDFIRE_MODE_NORMAL;
	}
}

static MfMenuRc rapidfire_load( void )
{
	if( ! mfMenuGetFilenameIsReady() ){
		if( mfMenuGetFilenameInit( "Open rapidfire preference", CGF_OPEN | CGF_FILEMUSTEXIST, "ms0:/", 128, 128 ) ){
			return MR_CONTINUE;
		} else{
			return MR_BACK;
		}
	} else{
		if( ! mfMenuGetFilename() ){
			char inipath[128 + 128];
			inipath[0] = '\0';
			
			if( mfMenuGetFilenameResult( inipath, sizeof( inipath ), NULL ) ){
				IniUID ini = inimgrNew();
				if( ini > 0 ){
					unsigned int err;
					if( ( err = inimgrLoad( ini, inipath ) ) == 0 ){
						if( ! rapidfire_ini_verck( ini ) ){
							gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FCCOLOR, "Initialization file is invalid or too lower version." );
							mfMenuWait( MFM_DISPLAY_MICROSEC_ERROR );
						} else{
							rapidfire_ini_load( ini );
							
							gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FGCOLOR, "Loaded from %s.", inipath );
							mfMenuWait( MFM_DISPLAY_MICROSEC_INFO );
						}
					} else{
						gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FCCOLOR, "Failed to load from %s: 0x%X", inipath, err );
						mfMenuWait( MFM_DISPLAY_MICROSEC_ERROR );
					}
					inimgrDestroy( ini );
				}
			}
			return MR_BACK;
		}
		return MR_CONTINUE;
	}
}

static MfMenuRc rapidfire_save( void )
{
	if( ! mfMenuGetFilenameIsReady() ){
		if( mfMenuGetFilenameInit( "Save rapidfire preference", CGF_SAVE | CGF_OVERWRITEPROMPT, "ms0:/", 128, 128 ) ){
			return MR_CONTINUE;
		} else{
			return MR_BACK;
		}
	} else{
		if( ! mfMenuGetFilename() ){
			char inipath[128 + 128];
			inipath[0] = '\0';
			
			if( mfMenuGetFilenameResult( inipath, sizeof( inipath ), NULL ) ){
				IniUID ini = inimgrNew();
				if( ini > 0 ){
					int i;
					unsigned int err;
					inimgrSetInt( ini, "default", RAPIDFIRE_DATA_SIGNATURE, RAPIDFIRE_DATA_VERSION );
					
					for( i = 0; i < MF_ARRAY_NUM( st_rfconf ); i++ ){
						inimgrSetString( ini, RAPIDFIRE_DATA_MODE_SECTION, st_rfconf[i].name, rapidfire_mode_to_string( st_rfconf[i].mode ) );
					}
					inimgrSetInt( ini, RAPIDFIRE_DATA_DELAY_SECTION, RAPIDFIRE_DID_RDELAY, st_release_delay );
					inimgrSetInt( ini, RAPIDFIRE_DATA_DELAY_SECTION, RAPIDFIRE_DID_PDELAY, st_press_delay );
					
					if( ( err = inimgrSave( ini, inipath ) ) == 0 ){
						gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FGCOLOR, "Saved to %s.", inipath );
						mfMenuWait( MFM_DISPLAY_MICROSEC_INFO );
					} else{
						gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FCCOLOR, "Failed to save to %s: 0x%X", inipath, err );
						mfMenuWait( MFM_DISPLAY_MICROSEC_ERROR );
					}
					inimgrDestroy( ini );
				}
			}
			return MR_BACK;
		}
		return MR_CONTINUE;
	}
}

void rapidfireLoadIni( IniUID ini, char *buf, size_t len )
{
	IniUID rfini;
	
	if( inimgrGetString( ini, "Rapidfire", "Default", buf, len ) <= 0 ) return;
	if( ( rfini = inimgrNew() ) > 0 ){
		if( inimgrLoad( rfini, buf ) == 0 && rapidfire_ini_verck( rfini ) ){
			rapidfire_ini_load( rfini );
		}
	}
	inimgrDestroy( rfini );
}

void rapidfireCreateIni( IniUID ini, char *buf, size_t len )
{
	inimgrSetString( ini, "Rapidfire", "Default", "" );
}

MfMenuRc rapidfireMenu( SceCtrlData *pad_data, void *argp )
{
	static int selected    = 0;
	
	if( sceKernelFindModuleByName( "sceHVNetfront_Module" ) != NULL ){
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Cannot use this function on the web-browser." );
		mfMenuWait( 1000000 );
		return MR_BACK;
	}
		
	if( ! st_callback.func ){
		gbPrint( gbOffsetChar(  3 ), gbOffsetLine(  4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Please choose a rapidfire mode per buttons." );
		gbPrint( gbOffsetChar( 33 ), gbOffsetLine( 22 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "NORMAL    : Standard control mode." );
		gbPrint( gbOffsetChar( 33 ), gbOffsetLine( 23 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "RAPID     : Rapidfire while pressing." );
		gbPrint( gbOffsetChar( 33 ), gbOffsetLine( 24 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "AUTO-RAPID: Always rapidfire." );
		gbPrint( gbOffsetChar( 33 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "HOLD      : Toggle the hold state." );
		gbPrint( gbOffsetChar( 33 ), gbOffsetLine( 26 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "AUTO-HOLD : Always to hold." );
		
		switch( mfMenuUniDraw( gbOffsetChar( 5 ), gbOffsetLine( 6 ), st_menu_table, MF_ARRAY_NUM( st_menu_table ), &selected, 0 ) ){
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

void rapidfireApply( void )
{
	int i;
	unsigned int *buttons          = NULL;
	unsigned int buttons_normal    = 0;
	unsigned int buttons_rapid     = 0;
	unsigned int buttons_autorapid = 0;
	unsigned int buttons_hold      = 0;
	unsigned int buttons_autohold  = 0;
	
	for( i = 0; i < MF_ARRAY_NUM( st_rfconf ); i++ ){
		switch( st_rfconf[i].mode ){
			case RAPIDFIRE_MODE_NORMAL:
				buttons = &buttons_normal;
				break;
			case RAPIDFIRE_MODE_SEMI_RAPID:
				buttons = &buttons_rapid;
				break;
			case RAPIDFIRE_MODE_AUTO_RAPID:
				buttons = &buttons_autorapid;
				break;
			case RAPIDFIRE_MODE_HOLD:
				buttons = &buttons_hold;
				break;
			case RAPIDFIRE_MODE_AUTO_HOLD:
				buttons = &buttons_autohold;
				break;
		}
		*buttons |= st_rfconf[i].button;
	}
	
	mfRapidfireSet( 0, buttons_normal,    MF_RAPIDFIRE_MODE_NORMAL,    0, 0 );
	mfRapidfireSet( 0, buttons_rapid,     MF_RAPIDFIRE_MODE_RAPID,     st_press_delay, st_release_delay );
	mfRapidfireSet( 0, buttons_autorapid, MF_RAPIDFIRE_MODE_AUTORAPID, st_press_delay, st_release_delay );
	mfRapidfireSet( 0, buttons_hold,      MF_RAPIDFIRE_MODE_HOLD,      0, 0 );
	mfRapidfireSet( 0, buttons_autohold,  MF_RAPIDFIRE_MODE_AUTOHOLD,  0, 0 );
}

static bool rapidfire_ini_verck( IniUID ini )
{
	int version;
	if( ! inimgrGetInt( ini, "default", RAPIDFIRE_DATA_SIGNATURE, (long *)&version ) || version <= RAPIDFIRE_DATA_REFUSE_VERSION ){
		return false;
	} else{
		return true;
	}
}

static void rapidfire_ini_load( IniUID ini )
{
	int i;
	char mode[16];
	for( i = 0; i < MF_ARRAY_NUM( st_rfconf ); i++ ){
		if( inimgrGetString( ini, RAPIDFIRE_DATA_MODE_SECTION, st_rfconf[i].name, mode, sizeof( mode ) ) ){
			st_rfconf[i].mode = rapidfire_string_to_mode( mode );
		} else{
				st_rfconf[i].mode = RAPIDFIRE_MODE_NORMAL;
			}
	}
	st_release_delay = RAPIDFIRE_DEFAULT_RELEASE_DELAY;
	st_press_delay   = RAPIDFIRE_DEFAULT_PRESS_DELAY;
	inimgrGetInt( ini, RAPIDFIRE_DATA_DELAY_SECTION, RAPIDFIRE_DID_RDELAY, (long *)&st_release_delay );
	inimgrGetInt( ini, RAPIDFIRE_DATA_DELAY_SECTION, RAPIDFIRE_DID_PDELAY, (long *)&st_press_delay );
}

/*
	remap.c
*/

#include "remap.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static bool remap_add( RemapData **remap );
static bool remap_edit( RemapData *cur_remap );
static bool remap_delete( RemapData *cur_remap );
static void remap_clear( RemapData **remap );
static MfMenuRc remap_menu_proc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg );
static MfMenuRc remap_menu_load( RemapData **remap );
static MfMenuRc remap_menu_save( RemapData **remap );
static MfMenuRc remap_menu_clear( RemapData **remap );
static void remap_load( InimgrCallbackMode mode, InimgrCallbackParams *params, char *buf, size_t buflen );
static void remap_save( InimgrCallbackMode mode, InimgrCallbackParams *params, char *buf, size_t buflen );

/*-----------------------------------------------
	ローカル変数と定数
-----------------------------------------------*/
static RemapData *st_remap;
static RemapData *st_work_remap;
static bool       st_clear_screen;

static MfMenuCallback st_callback;
static MfMenuItem st_menu_table[] = {
	{ "Load from MemoryStick", remap_menu_proc, &st_callback, { { .pointer = remap_menu_load }, { .pointer = NULL } } },
	{ "Save to MemoryStick",   remap_menu_proc, &st_callback, { { .pointer = remap_menu_save }, { .pointer = NULL } } },
	{ NULL },
	{ "Clear remap", remap_menu_proc, &st_callback,  { { .pointer = remap_menu_clear }, { .pointer = NULL } } }
};

/*=============================================*/

void remapTerm( void )
{
	remap_clear( &st_remap );
}

void remapMain( MfCallMode mode, SceCtrlData *pad, void *argp )
{
	if( st_remap ){
		RemapData *remap;
		SceCtrlData new_pad;
		unsigned int analog_direction, remove_buttons;
		
		analog_direction = ctrlpadUtilGetAnalogDirection( pad->Lx, pad->Ly, 0 );
		remove_buttons  = 0;
		new_pad.Buttons = 0;
		new_pad.Lx      = pad->Lx;
		new_pad.Ly      = pad->Ly;
		
		for( remap = st_remap; remap; remap = remap->next ){
			if( ( ( pad->Buttons | analog_direction ) & remap->realButtons ) == remap->realButtons ){
				if(
					( remap->realButtons & CTRLPAD_CTRL_ANALOG_UP )    ||
					( remap->realButtons & CTRLPAD_CTRL_ANALOG_RIGHT ) ||
					( remap->realButtons & CTRLPAD_CTRL_ANALOG_DOWN )  ||
					( remap->realButtons & CTRLPAD_CTRL_ANALOG_LEFT )
				){
					new_pad.Lx = 128;
					new_pad.Ly = 128;
				}
				
				if( remap->remapButtons & CTRLPAD_CTRL_ANALOG_UP    ) new_pad.Ly = 0;
				if( remap->remapButtons & CTRLPAD_CTRL_ANALOG_RIGHT ) new_pad.Lx = 255;
				if( remap->remapButtons & CTRLPAD_CTRL_ANALOG_DOWN  ) new_pad.Ly = 255;
				if( remap->remapButtons & CTRLPAD_CTRL_ANALOG_LEFT  ) new_pad.Lx = 0;
				remove_buttons  |= remap->realButtons;
				new_pad.Buttons |= remap->remapButtons;
			}
		}
		pad->Buttons &= ~remove_buttons;
		pad->Buttons |= new_pad.Buttons;
		pad->Lx      = new_pad.Lx;
		pad->Ly      = new_pad.Ly;
	}
}

MfMenuRc remapMenu( SceCtrlData *pad, void *arg )
{
	static int   selected = 0, column = REMAP_COLUMN_LIST, selected_menu = 0;
	unsigned int remap_count = 0;
	RemapData *remap_data = NULL, *cur_remap = NULL;
	
	if( st_callback.func ){
		MfMenuRc rc = ((RemapExtFunc)( st_callback.func ))( &st_remap );
		if( rc == MR_BACK ){
			st_callback.func = NULL;
			selected_menu    = 0;
			column           = REMAP_COLUMN_LIST;
			
			pad->Buttons = 0;
			mfMenuKeyRepeatReset();
			return MR_CONTINUE;
		} else if( st_clear_screen ){
			return MR_CONTINUE;
		}
	}
	
	gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Remapping buttons list." );
	
	if( ! st_remap ){
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 7 ), MFM_TEXT_FCCOLOR, MFM_TEXT_BGCOLOR, "<Press TRIANGLE(\x84) to add a new remap>" );
	} else{
		char         real_buttons[64];
		char         remap_buttons[64];
		unsigned int line, skip, viewlines;
		uint8_t      linepos;
		
		/* リマップされている個数を数える */
		for( remap_data = st_remap, remap_count = 0; remap_data->next; remap_data = remap_data->next, remap_count++ );
		
		if( selected < 0 ){
			selected = remap_count;
		} else if( selected > remap_count ){
			selected = 0;
		}
		
		/* スクロール */
		viewlines = REMAP_LINES_PER_PAGE;
		skip = mfMenuScroll( selected, viewlines, remap_count );
		for( remap_data = st_remap, line = 0; skip; remap_data = remap_data->next, line++, skip-- );
		
		/* 表示 */
		for( linepos = 0; viewlines && remap_data; remap_data = remap_data->next, line++, viewlines--, linepos++ ){
			if( ! remap_data->realButtons ) continue;
			
			real_buttons[0]  = '\0';
			remap_buttons[0] = '\0';
			
			mfMenuButtonsSymStr( remap_data->realButtons,  real_buttons,  sizeof( real_buttons ) );
			mfMenuButtonsSymStr( remap_data->remapButtons, remap_buttons, sizeof( remap_buttons ) );
			if( selected == line ){
				cur_remap = remap_data;
				gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 7 + linepos ), MFM_TEXT_FCCOLOR, MFM_TEXT_BGCOLOR, "%s: %s", real_buttons, remap_buttons );
			} else{
				gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 7 + linepos ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "%s: %s", real_buttons, remap_buttons );
			}
		}
	}
	
	if( column == REMAP_COLUMN_LIST ){
		gbLineRectRel( gbOffsetChar(  2 ), gbOffsetLine( 6 ), gbOffsetChar( 53 ), gbOffsetLine( REMAP_LINES_PER_PAGE + 2 ), MFM_TEXT_FCCOLOR );
		gbLineRectRel( gbOffsetChar( 56 ), gbOffsetLine( 6 ), gbOffsetChar( 23 ), gbOffsetLine( REMAP_LINES_PER_PAGE + 2 ), MFM_TEXT_FGCOLOR );
		mfMenuUniDraw( gbOffsetChar( 57 ), gbOffsetLine( 7 ), st_menu_table, MF_ARRAY_NUM( st_menu_table ), &selected_menu, MO_DISPLAY_ONLY );
		
		if( ! st_remap ){
			gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 31 ) + ( GB_CHAR_HEIGHT >> 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x86 = Back, START/HOME = Exit\n\x84 = Add, L/R = Switch column" );
		} else{
			gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 31 ) + ( GB_CHAR_HEIGHT >> 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x80\x82 = Move, \x83\x81 = PageMove, \x86 = Back, START/HOME = Exit\n\x85 = Edit remap buttons, \x84 = Add, \x87 = Delete, L/R = Switch column" );
		}
		
		if( mfMenuGetButtonsIsReady() ){
			if( mfMenuGetButtons() != MR_CONTINUE && ! st_work_remap->realButtons ){
				if( ! remap_delete( st_work_remap ) ) st_remap = NULL;
			}
		} else if( pad->Buttons ){
			if( pad->Buttons & PSP_CTRL_CROSS ){
				return MR_BACK;
			} else if( pad->Buttons & PSP_CTRL_TRIANGLE ){
				remap_add( &st_remap );
				selected = remap_count + 1;
			} else if( pad->Buttons & PSP_CTRL_CIRCLE ){
				remap_edit( cur_remap );
			} else if( pad->Buttons & PSP_CTRL_SQUARE ){
				if( ! remap_delete( cur_remap ) ) st_remap = NULL;
				if( selected > remap_count - 1 ) selected--;
			} else if( pad->Buttons & PSP_CTRL_UP ){
				selected--;
			} else if( pad->Buttons & PSP_CTRL_DOWN ){
				selected++;
			} else if( pad->Buttons & PSP_CTRL_RIGHT ){
				selected += REMAP_LINES_PER_PAGE >> 1;
				if( selected > remap_count ) selected = remap_count;
			} else if( pad->Buttons & PSP_CTRL_LEFT ){
				selected -= REMAP_LINES_PER_PAGE >> 1;
				if( selected < 0 ) selected = 0;
			} else if( pad->Buttons & ( PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER ) ){
				column = REMAP_COLUMN_MENU;
			}
		}
	} else{
		gbLineRectRel( gbOffsetChar(  2 ), gbOffsetLine( 6 ), gbOffsetChar( 53 ), gbOffsetLine( REMAP_LINES_PER_PAGE + 2 ), MFM_TEXT_FGCOLOR );
		gbLineRectRel( gbOffsetChar( 56 ), gbOffsetLine( 6 ), gbOffsetChar( 23 ), gbOffsetLine( REMAP_LINES_PER_PAGE + 2 ), MFM_TEXT_FCCOLOR );
		
		mfMenuUniDraw( gbOffsetChar( 57 ), gbOffsetLine( 7 ), st_menu_table, MF_ARRAY_NUM( st_menu_table ), &selected_menu, 0 );
		
		if( st_callback.func ) st_clear_screen = true;
		
		if( pad->Buttons & PSP_CTRL_CROSS ){
			return MR_BACK;
		} else if( pad->Buttons & ( PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER ) ){
			column = REMAP_COLUMN_LIST;
		}
	}
	
	return MR_CONTINUE;
}

static bool remap_add( RemapData **remap )
{
	RemapData *cur_remap;
	
	if( ! *remap ){
		*remap = (RemapData *)memsceCalloc( 1, sizeof( RemapData ) );
		if( ! *remap ) return false;
		
		cur_remap = *remap;
	} else{
		cur_remap = *remap;
		for( cur_remap = *remap; cur_remap->next; cur_remap = cur_remap->next );
		cur_remap->next = (RemapData *)memsceCalloc( 1, sizeof( RemapData ) );
		if( ! cur_remap->next ) return false;
		
		cur_remap->next->prev = cur_remap;
		cur_remap = cur_remap->next;
	}
	
	if( mfMenuGetButtonsInit( "Set real buttons", &(cur_remap->realButtons), 0x0000FFFF ) != MR_ENTER ) return false;
	
	st_work_remap = cur_remap;
	
	return true;
}

static bool remap_edit( RemapData *cur_remap )
{
	if( ! cur_remap ){
		return false;
	} else if( mfMenuGetButtonsInit( "Set remap buttons", &(cur_remap->remapButtons), 0x0000FFFF ) != MR_ENTER ){
		return false;
	}
	
	st_work_remap = cur_remap;
	
	return true;
}

static bool remap_delete( RemapData *cur_remap )
{
	if( ! cur_remap ){
		return false;
	} else if( cur_remap->prev ){
		cur_remap->prev->next = cur_remap->next;
		if( cur_remap->next ){
			cur_remap->next->prev = cur_remap->prev;
		}
		memsceFree( cur_remap );
	} else{
		if( ! cur_remap->next ){
			memsceFree( cur_remap );
			return false;
		} else{
			RemapData *rm_remap = cur_remap->next;
			*cur_remap = *rm_remap;
			cur_remap->prev = NULL;
			if( cur_remap->next ) cur_remap->next->prev = cur_remap;
			memsceFree( rm_remap );
		}
	}
	
	return true;
}

static MfMenuRc remap_menu_load( RemapData **remap )
{
	if( ! mfMenuGetFilenameIsReady() ){
		if( mfMenuGetFilenameInit( "Open remap preference", CGF_OPEN | CGF_FILEMUSTEXIST, "ms0:/", 128, 128 ) ){
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
					inimgrSetCallback( ini, REMAP_DATA_SECTION, remap_load );
					if( ( err = inimgrLoad( ini, inipath ) ) == 0 ){
						int version;
						if( ! inimgrGetInt( ini, "default", REMAP_DATA_SIGNATURE, (long *)&version ) || version <= REMAP_DATA_REFUSE_VERSION ){
							gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FCCOLOR, "remap file is invalid or too lower version." );
							mfMenuWait( MFM_DISPLAY_MICROSEC_ERROR );
						} else{
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

static MfMenuRc remap_menu_save( RemapData **remap )
{
	if( ! mfMenuGetFilenameIsReady() ){
		if( ! *remap ){
			gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "No remap entry." );
			mfMenuWait( MFM_DISPLAY_MICROSEC_INFO );
			return MR_BACK;
		}
		
		if( mfMenuGetFilenameInit( "Save remap preference", CGF_SAVE | CGF_FILEMUSTEXIST, "ms0:/", 128, 128 ) ){
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
					inimgrSetInt( ini, "default", REMAP_DATA_SIGNATURE, REMAP_DATA_VERSION );
					inimgrAddSection(  ini, REMAP_DATA_SECTION );
					inimgrSetCallback( ini, REMAP_DATA_SECTION, remap_save );
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
	}}

static void remap_load( InimgrCallbackMode mode, InimgrCallbackParams *params, char *buf, size_t buflen )
{
	RemapData *remap = NULL;
	char *remap_line[REMAP_INILINE_ALL_COLUMNS], *key, *value;
	char *saveptr;
	int version = 0, prev_seq = -1, cur_seq;
	
	if( ! inimgrGetInt( params->uid, "default", REMAP_DATA_SIGNATURE, (long *)&version ) || version <= REMAP_DATA_REFUSE_VERSION ) return;
	
	if( st_remap ) remap_clear( &st_remap );
	
	while( inimgrCbReadln( params, buf, buflen ) ){
		inimgrParseEntry( buf, &key, &value );
		
		remap_line[REMAP_INILINE_SEQ]    = strtok_r( key,  ".", &saveptr );
		remap_line[REMAP_INILINE_TARGET] = strtok_r( NULL, ".", &saveptr );
		
		if( ! remap_line[REMAP_INILINE_TARGET] ) return;
		
		cur_seq = strtol( remap_line[REMAP_INILINE_SEQ], NULL, 10 );
		
		if( prev_seq < cur_seq ){
			if( ! st_remap ){
				st_remap = (RemapData *)memsceMalloc( sizeof( RemapData ) );
				if( ! st_remap ) return;
				
				remap = st_remap;
				remap->prev = NULL;
				remap->next = NULL;
			} else{
				remap->next = (RemapData *)memsceMalloc( sizeof( RemapData ) );
				if( ! remap->next ) return;
				
				remap->next->prev = remap;
				remap = remap->next;
				remap->next = NULL;
			}
			remap->realButtons  = 0;
			remap->remapButtons = 0;
			
			prev_seq = cur_seq;
		} else if( cur_seq < prev_seq ){
			return;
		} else if( ! st_remap ){
			continue;
		}
		
		if( strcmp( remap_line[REMAP_INILINE_TARGET], REMAP_REAL_BUTTONS ) == 0 ){
			remap->realButtons = ctrlpadUtilStringToButtons( value );
		} else if( strcmp( remap_line[REMAP_INILINE_TARGET], REMAP_REMAP_BUTTONS ) == 0 ){
			remap->remapButtons = ctrlpadUtilStringToButtons( value );
		} else{
			return;
		}
	}
}

static void remap_save( InimgrCallbackMode mode, InimgrCallbackParams *params, char *buf, size_t buflen )
{
	RemapData *remap;
	char buttons[128];
	unsigned int seq = 0, len = 0;
	
	for( remap = st_remap; remap; remap = remap->next, seq++ ){
		if( remap->realButtons ){
			ctrlpadUtilButtonsToString( remap->realButtons, buttons, sizeof( buttons ) );
			len = snprintf( buf, buflen, "%d.%s = %s", seq, REMAP_REAL_BUTTONS, buttons );
			inimgrCbWriteln( params, buf, len );
			
			if( remap->remapButtons ){
				ctrlpadUtilButtonsToString( remap->remapButtons, buttons, sizeof( buttons ) );
				len = snprintf( buf, buflen, "%d.%s = %s", seq, REMAP_REMAP_BUTTONS, buttons );
				inimgrCbWriteln( params, buf, len );
			}
		}
	}
}


static MfMenuRc remap_menu_clear( RemapData **remap )
{
	st_clear_screen = false;
	
	if( ! mfMenuMessageIsReady() ){
		if( ! mfMenuMessageInit( "Warning", "Clearing all remap entries.\nAre you sure?", true ) ) return MR_BACK;
	} else{
		if( ! mfMenuMessage() ){
			if( mfMenuMessageResult() ){
				remap_clear( remap );
			}
			return MR_BACK;
		}
	}
	return MR_CONTINUE;
}

static void remap_clear( RemapData **remap )
{
	if( ! (*remap) ){
		return;
	} else if( ! (*remap)->next ){
		memsceFree( *remap );
		*remap = NULL;
	} else{
		RemapData *cur_remap;
		
		for( cur_remap = (*remap)->next; cur_remap->next; cur_remap = cur_remap->next ){
			memsceFree( cur_remap->prev );
		}
		memsceFree( cur_remap->prev );
		memsceFree( cur_remap );
		*remap = NULL;
	}
}

static MfMenuRc remap_menu_proc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg )
{
	if( signal == MS_QUERY_USAGE ){
		mfMenuUsage( "\x85 = Enter, L/R = Switch column" );
		return MR_CONTINUE;
	} else{
		return mfMenuDefCallbackProc( signal, pad, var, value, arg );
	}
}

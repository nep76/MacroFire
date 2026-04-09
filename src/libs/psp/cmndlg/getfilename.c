/*
	getfilename.c
*/

#include "getfilename.h"

static u32 fgcolor = 0xffffffff;
static u32 bgcolor = 0xff000000;
static u32 fccolor = 0xff0000ff;
static u32 dircolor = 0xffff0000;
static int st_all_entry_num;

static enum getfilename_control_rc getfilename_control_list( SceCtrlLatch *pad_latch, char *cwd, CmndlgOpenFilename *cofn, DirhUID dird, int *selected )
{
	if( pad_latch->uiMake & PSP_CTRL_CIRCLE ){
		unsigned int attr = dirhGetFileType( dird, *selected );
		char *name        = dirhGetFileName( dird, *selected );
		
		if( strlen( cwd ) + 1 > cofn->dirPathMax ){
			return CMNDLG_GETFILENAME_CONTROL_RC_QUIT;
		}
		strcpy( cofn->dirPath, cwd );
		
		if( attr == FIO_SO_IFDIR ){
			if( strlen( cofn->dirPath ) + 1 + strlen( name ) + 1 > cofn->dirPathMax ){
				return CMNDLG_GETFILENAME_CONTROL_RC_QUIT;
			}
			strcat( cofn->dirPath, "/" );
			strcat( cofn->dirPath, name );
			return CMNDLG_GETFILENAME_CONTROL_RC_CONTINUE;
		} else if( attr == FIO_SO_IFREG ){
			if( strlen( name ) + 1 > cofn->fileNameMax ){
				return CMNDLG_GETFILENAME_CONTROL_RC_QUIT;
			}
			strcpy( cofn->fileName, name );
			return CMNDLG_GETFILENAME_CONTROL_RC_REDRAW_NAME;
		}
	} else if( pad_latch->uiMake & PSP_CTRL_TRIANGLE ){
		if( strlen( cofn->dirPath ) + 5 > cofn->dirPathMax ){
			return CMNDLG_GETFILENAME_CONTROL_RC_QUIT;
		}
		strcat( cofn->dirPath, "/../" );
		return CMNDLG_GETFILENAME_CONTROL_RC_CONTINUE;
	} else if( pad_latch->uiMake & PSP_CTRL_UP ){
		*selected -= 1;
	} else if( pad_latch->uiMake & PSP_CTRL_DOWN ){
		*selected += 1;
	} else if( pad_latch->uiMake & PSP_CTRL_LEFT ){
		*selected -= CMNDLG_GETFILENAME_ITEMS_PER_PAGE;
	} else if( pad_latch->uiMake & PSP_CTRL_RIGHT ){
		*selected += CMNDLG_GETFILENAME_ITEMS_PER_PAGE;
	}
	
	if( *selected < 0 ){
		if( pad_latch->uiMake & PSP_CTRL_UP ){
			*selected = st_all_entry_num - 1;
		} else{
			*selected = 0;
		}
	}
	if( *selected > st_all_entry_num - 1 ){
		if( pad_latch->uiMake & PSP_CTRL_DOWN ){
			*selected = 0;
		} else{
			*selected = st_all_entry_num - 1;
		}
	}
	
	return CMNDLG_GETFILENAME_CONTROL_RC_NOOP;
}

static enum getfilename_control_rc getfilename_control_name( SceCtrlLatch *pad_latch, char *cwd, CmndlgOpenFilename *cofn, DirhUID dird )
{
	enum getfilename_control_rc rc = CMNDLG_GETFILENAME_CONTROL_RC_NOOP;
	
	if( pad_latch->uiMake & PSP_CTRL_CIRCLE ){
		SoskRc sosk_rc;
		for( ;; ){
			sosk_rc = soskRun(
				"TESTDESC",
				CMNDLG_GETFILENAME_SX( 0 ),
				CMNDLG_GETFILENAME_SY( 0 ),
				fgcolor,
				bgcolor,
				fccolor,
				cofn->fileName,
				cofn->fileNameMax
			);
			
			if( sosk_rc != SR_ACCEPT ){
				cofn->fileName[0] = '\0';
				break;
			} else{
				break;
			}
		}
		rc = CMNDLG_GETFILENAME_CONTROL_RC_CONTINUE;
	}
	
	return rc;
}

static enum getfilename_rc getfilename( char *title, CmndlgOpenFilename *cofn, enum getfilename_mode mode )
{
	enum getfilename_focus focus = CMNDLG_GETFILENAME_FOCUS_LIST;
	enum getfilename_rc rc       = CMNDLG_GETFILENAME_RC_CONTINUE;
	unsigned int control_rc      = 0;
	SceCtrlLatch pad_latch;
	DirhUID dird;
	
	u32 *list_frame_color = NULL;
	u32 *name_frame_color = NULL;
	
	char *fname = NULL;
	char *cwd   = NULL;
	int entry_pos = 0, prev_entry_start = -1, drawline, selected = 0, cnt;
	bool redraw_list = true, redraw_name = true;
	
	/* 枠描画 */
	blitFillRect( CMNDLG_GETFILENAME_SX( 0 ), CMNDLG_GETFILENAME_SY( 0 ), CMNDLG_GETFILENAME_EX( 0 ), CMNDLG_GETFILENAME_EY( 0 ), bgcolor );
	blitLineRect( CMNDLG_GETFILENAME_SX( 0 ), CMNDLG_GETFILENAME_SY( 0 ), CMNDLG_GETFILENAME_EX( 0 ), CMNDLG_GETFILENAME_EY( 0 ), fgcolor );
	
	/* タイトル描画 */
	blitString( CMNDLG_GETFILENAME_SX( 1 ), CMNDLG_GETFILENAME_SY( 1 ), fgcolor, bgcolor, title );
	
	dird = dirhOpen( cofn->dirPath, DIRH_OPT_ADD_DIR_SLASH );
	if( dirhGetLastError( dird ) ){
		blitStringf( CMNDLG_GETFILENAME_SX( 1 ), CMNDLG_GETFILENAME_SY( 3 ), fgcolor, bgcolor, "dirh error: Failed to open %s: %x-%x", cofn->dirPath, dirhGetLastError( dird ), dirhGetLastSystemError( dird ) );
		sceDisplayWaitVblankStart();
		sceKernelDelayThread( 3000000 );
		return CMNDLG_GETFILENAME_RC_CANCEL;
	}
	
	/* カレントディレクトリ描画 */
	cwd  = dirhGetCwd( dird );
	if( mode == CMNDLG_GETFILENAME_MODE_SAVE ){
		blitStringf( CMNDLG_GETFILENAME_SX( 1 ), CMNDLG_GETFILENAME_SY( 3 ), fgcolor, bgcolor, "Save in: %s", cwd );
	} else if( mode == CMNDLG_GETFILENAME_MODE_OPEN ){
		blitStringf( CMNDLG_GETFILENAME_SX( 1 ), CMNDLG_GETFILENAME_SY( 3 ), fgcolor, bgcolor, "Look in: %s", cwd );
	}
	
	memset( &pad_latch, 0, sizeof( SceCtrlLatch ) );
	
	st_all_entry_num = dirhGetAllEntryCount( dird );
	
	for( ;; ){
		sceCtrlReadLatch( &pad_latch );
		
		if( pad_latch.uiMake & PSP_CTRL_CROSS || pad_latch.uiMake & PSP_CTRL_HOME ){
			rc = CMNDLG_GETFILENAME_RC_CANCEL;
			break;
		} else if( pad_latch.uiMake & PSP_CTRL_START ){
			if( strpbrk( cofn->fileName, "\\/:*?\"<>|" ) ){
				cmndlgMsg(
					CMNDLG_GETFILENAME_SX( 15 ),
					CMNDLG_GETFILENAME_SY( 11 ),
					0,
					"Filename contains illegal characters\n\nIllegal characters = \\/:?\"<>|"
				);
				rc = CMNDLG_GETFILENAME_RC_CONTINUE;
				break;
			} else if( cofn->fileName[0] == '\0' ){
				cmndlgMsg(
					CMNDLG_GETFILENAME_SX( 23 ),
					CMNDLG_GETFILENAME_SY( 11 ),
					0,
					"Filename is empty"
				);
				rc = CMNDLG_GETFILENAME_RC_CONTINUE;
				break;
			} else{
				rc = CMNDLG_GETFILENAME_RC_ACCEPT;
				break;
			}
		} else if( pad_latch.uiMake & PSP_CTRL_LTRIGGER || pad_latch.uiMake & PSP_CTRL_RTRIGGER ){
			/* キー操作部を書き換えるためにその領域をクリア */
			blitFillRect( CMNDLG_GETFILENAME_SX( 1 ), CMNDLG_GETFILENAME_EY( 3 ), CMNDLG_GETFILENAME_EX( 1 ), CMNDLG_GETFILENAME_EY( 1 ), bgcolor );
			
			if( focus == CMNDLG_GETFILENAME_FOCUS_LIST ){
				focus = CMNDLG_GETFILENAME_FOCUS_NAME;
			} else if( focus == CMNDLG_GETFILENAME_FOCUS_NAME ){
				focus = CMNDLG_GETFILENAME_FOCUS_LIST;
			}
		}
		
		if( focus == CMNDLG_GETFILENAME_FOCUS_LIST ){
			/* 枠の色を設定 */
			list_frame_color = &fccolor;
			name_frame_color = &fgcolor;
			/* キー操作描画 */
			blitString( CMNDLG_GETFILENAME_SX( 1 ), CMNDLG_GETFILENAME_EY( 3 ), fgcolor, bgcolor, "\x80 = Up, \x82 = Down, \x83 = PageUp, \x81 = PageDown, LR = MoveFocus" );
			blitString( CMNDLG_GETFILENAME_SX( 1 ), CMNDLG_GETFILENAME_EY( 2 ), fgcolor, bgcolor, "\x84 = ParentDirectory, \x85 = Enter, START = Accept, \x86 = Cancel" );
			control_rc = getfilename_control_list( &pad_latch, cwd, cofn, dird, &selected );
		} else if( focus == CMNDLG_GETFILENAME_FOCUS_NAME ){
			/* 枠の色を設定 */
			list_frame_color = &fgcolor;
			name_frame_color = &fccolor;
			/* キー操作描画 */
			blitString( CMNDLG_GETFILENAME_SX( 1 ), CMNDLG_GETFILENAME_EY( 3 ), fgcolor, bgcolor, "LR = MoveFocus, \x85 = Input, START = Accept, \x86 = Cancel" );
			control_rc = getfilename_control_name( &pad_latch, cwd, cofn, dird );
		}
		
		if( control_rc & CMNDLG_GETFILENAME_CONTROL_RC_QUIT )       { rc = CMNDLG_GETFILENAME_RC_CANCEL; break; }
		if( control_rc & CMNDLG_GETFILENAME_CONTROL_RC_CONTINUE )   { rc = CMNDLG_GETFILENAME_RC_CONTINUE; break; }
		if( control_rc & CMNDLG_GETFILENAME_CONTROL_RC_REDRAW_LIST ){ redraw_list = true; }
		if( control_rc & CMNDLG_GETFILENAME_CONTROL_RC_REDRAW_NAME ){ redraw_name = true; }
		
		entry_pos = selected - ( CMNDLG_GETFILENAME_ITEMS_PER_PAGE >> 1 );
		if( CMNDLG_GETFILENAME_ITEMS_PER_PAGE >= ( st_all_entry_num - entry_pos ) + 1 ){
			entry_pos = st_all_entry_num - CMNDLG_GETFILENAME_ITEMS_PER_PAGE;
			redraw_list = false;
		}
		if( entry_pos < 0 )                { entry_pos = 0; }
		if( prev_entry_start != entry_pos ){ redraw_list = true; }
		prev_entry_start = entry_pos;
		dirhSeek( dird, DW_SEEK_SET, entry_pos );
		
		/* ファイルリスト枠を描画 */
		blitLineRect( CMNDLG_GETFILENAME_SX( 1 ), CMNDLG_GETFILENAME_SY( 4 ), CMNDLG_GETFILENAME_EX( 1 ), CMNDLG_GETFILENAME_SY( 4 + CMNDLG_GETFILENAME_ITEMS_PER_PAGE + 2 ), *list_frame_color );
		for( drawline = CMNDLG_GETFILENAME_ITEMS_PER_PAGE, cnt = 0; drawline; drawline--, entry_pos++, cnt++ ){
			fname = dirhRead( dird );
			if( ! fname ) break;
			
			if( redraw_list ) blitFillRect( CMNDLG_GETFILENAME_SX( 2 ), CMNDLG_GETFILENAME_SY( 5 + cnt ), CMNDLG_GETFILENAME_EX( 2 ),CMNDLG_GETFILENAME_SY( 5 + cnt + 2 ), bgcolor );
			
			if( selected == entry_pos ){
				blitString( CMNDLG_GETFILENAME_SX( 2 ), CMNDLG_GETFILENAME_SY( 5 + cnt ), fccolor, bgcolor, fname );
			} else if( dirhGetFileType( dird, dirhTell( dird ) - 1 ) == FIO_SO_IFDIR ){
				blitString( CMNDLG_GETFILENAME_SX( 2 ), CMNDLG_GETFILENAME_SY( 5 + cnt ), dircolor, bgcolor, fname );
			} else{
				blitString( CMNDLG_GETFILENAME_SX( 2 ), CMNDLG_GETFILENAME_SY( 5 + cnt ), fgcolor, bgcolor, fname );
			}
		}
		if( redraw_list && drawline ) blitFillRect( CMNDLG_GETFILENAME_SX( 2 ), CMNDLG_GETFILENAME_SY( 5 + cnt ), CMNDLG_GETFILENAME_EX( 2 ), CMNDLG_GETFILENAME_SY( 5 + cnt + drawline ), bgcolor );
		
		/* ファイル名枠を描画 */
		if( redraw_name ) blitFillRect( CMNDLG_GETFILENAME_SX( 1 ), CMNDLG_GETFILENAME_SY( 4 + CMNDLG_GETFILENAME_ITEMS_PER_PAGE + 3 ), CMNDLG_GETFILENAME_EX( 1 ), CMNDLG_GETFILENAME_SY( 4 + CMNDLG_GETFILENAME_ITEMS_PER_PAGE + 3 + 3 ), bgcolor );
		blitLineRect( CMNDLG_GETFILENAME_SX( 1 ), CMNDLG_GETFILENAME_SY( 4 + CMNDLG_GETFILENAME_ITEMS_PER_PAGE + 3 ), CMNDLG_GETFILENAME_EX( 1 ), CMNDLG_GETFILENAME_SY( 4 + CMNDLG_GETFILENAME_ITEMS_PER_PAGE + 3 + 3 ), *name_frame_color );
		blitStringf( CMNDLG_GETFILENAME_SX( 2 ), CMNDLG_GETFILENAME_SY( 4 + CMNDLG_GETFILENAME_ITEMS_PER_PAGE + 4 ), fgcolor, bgcolor, "File Name: %s", cofn->fileName );
		
		redraw_list = false;
		redraw_name = false;
		
		sceDisplayWaitVblankStart();
	}
	
	dirhClose( dird );
	
	return rc;
}

CmndlgGetFilenameRc cmndlgGetSaveFilename( char *title, char *initdir, CmndlgOpenFilename *cofn )
{
	CmndlgGetFilenameRc ret;
	
	if( strlen( initdir ) + 1 > cofn->dirPathMax ){
		return -1;
	} else{
		strcpy( cofn->dirPath, initdir );
	}
	
	do {
		ret = getfilename( title, cofn, CMNDLG_GETFILENAME_MODE_SAVE );
	} while( ret == CMNDLG_GETFILENAME_RC_CONTINUE );
	
	if( ret == CMNDLG_GETFILENAME_RC_ACCEPT ){
		return CMNDLG_GETFILENAME_ACCEPT;
	} else{
		return CMNDLG_GETFILENAME_CANCEL;
	}
}

CmndlgGetFilenameRc cmndlgGetOpenFilename( char *title, char *initdir, CmndlgOpenFilename *cofn )
{
	CmndlgGetFilenameRc ret;
	
	if( strlen( initdir ) + 1 > cofn->dirPathMax ){
		return -1;
	} else{
		strcpy( cofn->dirPath, initdir );
	}
	
	do {
		ret = getfilename( title, cofn, CMNDLG_GETFILENAME_MODE_OPEN );
	} while( ret == CMNDLG_GETFILENAME_RC_CONTINUE );
	
	if( ret == CMNDLG_GETFILENAME_RC_ACCEPT ){
		return CMNDLG_GETFILENAME_ACCEPT;
	} else{
		return CMNDLG_GETFILENAME_CANCEL;
	}
}

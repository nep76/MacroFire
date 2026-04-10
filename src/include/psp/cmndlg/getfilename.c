/*
	getfilename.c
*/

#include "getfilename.h"


/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static void cmndlg_get_filename_draw_ui( CmndlgGetFilenameParams *params );
static bool cmndlg_get_filename_move_parentdir( char *path );
static bool cmndlg_get_filename_message_start( const char *title, const char *message );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static CmndlgGetFilenameParams *st_params;
static CtrlpadParams          st_cp_params;
static bool                   st_show_msg = false;
static bool                   st_input    = false;
static bool                   st_chdir    = true;
static int                    st_all_entries;
static DirhUID                st_dirh;

/*=============================================*/

CmndlgGetFilenameParams *cmndlgGetFilenameGetParams( void )
{
	return st_params;
}

CmndlgState cmndlgGetFilenameGetStatus( void )
{
	if( ! st_params ){
		return CMNDLG_NONE;
	} else{
		return st_params->base.state;
	}
}

int cmndlgGetFilenameStart( CmndlgGetFilenameParams *params )
{
	int i;
	
	if( st_params || ! params ) return -1;
	
	st_params = params;
	
	st_params->base.state = CMNDLG_INIT;
	
	st_params->base.tempBuffer = memsceMalloc( sizeof( struct cmndlg_get_filename_tempdata ) * st_params->numberOfData );
	if( ! st_params->base.tempBuffer ) return -2;
	
	for( i = 0; i < st_params->numberOfData; i++ ){
		pathexpandFromCwd( st_params->data[i].initPath, st_params->data[i].path, st_params->data[i].pathMax );
		((struct cmndlg_get_filename_tempdata *)(st_params->base.tempBuffer))[i].selected = 0;
		((struct cmndlg_get_filename_tempdata *)(st_params->base.tempBuffer))[i].focus    = CMNDLG_GET_FILENAME_FOCUS_LIST;
	}
	
	ctrlpadInit( &st_cp_params );
	ctrlpadSetRepeatButtons( &st_cp_params, PSP_CTRL_UP | PSP_CTRL_RIGHT | PSP_CTRL_DOWN | PSP_CTRL_LEFT );
	
	st_params->base.state = CMNDLG_VISIBLE;
	
	return 0;
}

int cmndlgGetFilenameUpdate( void )
{
	CmndlgGetFilenameData *selected_data = &(st_params->data[st_params->selectDataNumber]);
	struct cmndlg_get_filename_tempdata *tempdata = &(((struct cmndlg_get_filename_tempdata *)(st_params->base.tempBuffer))[st_params->selectDataNumber]);
	SceCtrlData pad_data;
	pad_data.Buttons = ctrlpadGetData( &st_cp_params, &pad_data, CTRLPAD_IGNORE_ANALOG_DIRECTION );
	
	/* ディレクトリリスト取得 */
	if( st_chdir ){
		int ret;
		if( st_dirh ){
			ret = dirhChdir( st_dirh, selected_data->path );
		} else{
			st_dirh = dirhNew( selected_data->path, DIRH_OPT_ADD_DIR_SLASH );
			ret = st_dirh;
		}
		if( ret < 0 ){
			st_params->base.state = CMNDLG_SHUTDOWN;
			return -1;
		}
		st_chdir = false;
		st_all_entries = dirhGetAllEntryCount( st_dirh );
		tempdata->selected = 0;
	}
	
	cmndlg_get_filename_draw_ui( st_params );
	
	if( st_show_msg ){
		CmndlgState state;
		
		cmndlgMessageUpdate();
		state = cmndlgMessageGetStatus();
		if( state == CMNDLG_SHUTDOWN ){
			CmndlgMessageParams *msg_params = cmndlgMessageGetParams();
			cmndlgMessageShutdownStart();
			memsceFree( msg_params );
			ctrlpadUpdateData( &st_cp_params );
			st_show_msg = false;
		}
		return 0;
	} else if( st_input ){
		CmndlgState state;
		
		cmndlgSoskUpdate();
		state = cmndlgSoskGetStatus();
		if( state == CMNDLG_SHUTDOWN ){
			CmndlgSoskParams *sosk_params = cmndlgSoskGetParams();
			cmndlgSoskShutdownStart();
			memsceFree( sosk_params );
			ctrlpadUpdateData( &st_cp_params );
			st_input = false;
		}
		return 0;
	}
	
	/* 共通コントロール */
	if( pad_data.Buttons & PSP_CTRL_SELECT ){
		st_show_msg = cmndlg_get_filename_message_start( "Usage", "L/R   = MoveFocus\nSTART = Accept\n\x86     = Cancel\n\nFilelist Usage\n\n  \x80\x82 = Move\n  \x85  = Enter\n  \x84  = Parent Directory\n\nFilename Usage\n\n  \x85 = Input" );
	} else if( pad_data.Buttons & PSP_CTRL_CROSS ){
		st_params->rc         = CMNDLG_CANCEL;
		st_params->base.state = CMNDLG_SHUTDOWN;
	} else if( pad_data.Buttons & PSP_CTRL_START ){
		if( strpbrk( selected_data->name, "\\/:*?\"<>|" ) ){
			st_show_msg = cmndlg_get_filename_message_start( "Invalid filename", "Filename contains illegal characters\n\nIllegal characters = \\/:?\"<>|" );
		} else if( selected_data->name[0] == '\0' ){
			st_show_msg = cmndlg_get_filename_message_start( "Invalid filename", "Filename is empty" );
		} else{
			st_params->rc         = CMNDLG_ACCEPT;
			st_params->base.state = CMNDLG_SHUTDOWN;
		}
	} else if( pad_data.Buttons & PSP_CTRL_LTRIGGER || pad_data.Buttons & PSP_CTRL_RTRIGGER ){
		if( tempdata->focus == CMNDLG_GET_FILENAME_FOCUS_LIST ){
			tempdata->focus = CMNDLG_GET_FILENAME_FOCUS_NAME;
		} else{
			tempdata->focus = CMNDLG_GET_FILENAME_FOCUS_LIST;
		}
	} else if( pad_data.Buttons & PSP_CTRL_CIRCLE ){
		if( tempdata->focus == CMNDLG_GET_FILENAME_FOCUS_LIST ){
			unsigned int attr = dirhGetFileType( st_dirh, tempdata->selected );
			char *name        = dirhGetFileName( st_dirh, tempdata->selected );
			
			if( strcmp( name, "../" ) == 0 ){
				if( cmndlg_get_filename_move_parentdir( selected_data->path ) ) st_chdir = true;
			} else if( attr == FIO_SO_IFDIR ){
				if( strlen( selected_data->path ) + strlen( name ) + 2 > selected_data->pathMax ){
					//エラー
					return -1;
				}
				strcat( selected_data->path, "/" );
				strcat( selected_data->path, name );
				st_chdir = true;
			} else{
				strutilSafeCopy( selected_data->name, name, selected_data->nameMax );
			}
		} else{
			CmndlgSoskParams *sosk_params = (CmndlgSoskParams *)memsceMalloc( sizeof( CmndlgSoskParams ) );
			if( ! sosk_params ){
				return -1;
			}
			
			sosk_params->title          = "Input filename";
			sosk_params->text           = selected_data->name;
			sosk_params->textMax        = selected_data->nameMax;
			sosk_params->rc             = 0;
			sosk_params->options        = CMNDLG_SOSK_DISPLAY_CENTER;
			sosk_params->ui.x           = 0;
			sosk_params->ui.y           = 0;
			sosk_params->ui.fgTextColor = st_params->ui.fgTextColor;
			sosk_params->ui.fcTextColor = st_params->ui.fcTextColor;
			sosk_params->ui.bgTextColor = st_params->ui.bgTextColor;
			sosk_params->ui.bgColor     = 0xdd000000;
			sosk_params->ui.borderColor = 0x0;
			
			if( ! sosk_params->text ){
				memsceFree( sosk_params );
				return -1;
			}
			
			if( cmndlgSoskStart( sosk_params ) ){
				memsceFree( sosk_params );
				return -1;
			}
			st_input = true;
		}
	}
	
	/* ファイルリストコントロール */
	else if( pad_data.Buttons & PSP_CTRL_DOWN && tempdata->focus == CMNDLG_GET_FILENAME_FOCUS_LIST ){
		if( tempdata->selected >= st_all_entries - 1 ){
			tempdata->selected = 0;
		} else{
			tempdata->selected++;
		}
	} else if( pad_data.Buttons & PSP_CTRL_UP && tempdata->focus == CMNDLG_GET_FILENAME_FOCUS_LIST ){
		if( tempdata->selected <= 0 ){
			tempdata->selected = st_all_entries - 1;
		} else{
			tempdata->selected--;
		}
	} else if( pad_data.Buttons & PSP_CTRL_TRIANGLE && tempdata->focus == CMNDLG_GET_FILENAME_FOCUS_LIST ){
		if( cmndlg_get_filename_move_parentdir( selected_data->path ) ) st_chdir = true;
	}
	
	return 0;
}

int cmndlgGetFilenameShutdownStart( void )
{
	if( st_params->base.state != CMNDLG_SHUTDOWN ){
		st_params->rc         = CMNDLG_CANCEL;
		st_params->base.state = CMNDLG_SHUTDOWN;
	}
	
	if( st_dirh <= 0 ){
		st_dirh = 0;
	} else{
		dirhDestroy( st_dirh );
		st_dirh  = 0;
		st_chdir = true;
	}
	
	memsceFree( st_params->base.tempBuffer );
	st_params->base.tempBuffer = NULL;
	st_params = NULL;
	
	return 0;
}

static void cmndlg_get_filename_draw_ui( CmndlgGetFilenameParams *params )
{
	int drawline, rest_lines, line_number;
	int dircnt = dirhGetDirEntryCount( st_dirh );
	char *dirent_name;
	struct cmndlg_get_filename_tempdata *tempdata = &(((struct cmndlg_get_filename_tempdata *)(params->base.tempBuffer))[params->selectDataNumber]);
	
	/* 枠を描画 */
	gbFillRectRel( params->ui.x, params->ui.y, gbOffsetChar( params->ui.w ), gbOffsetLine( params->ui.h ), params->ui.bgColor );
	gbLineRectRel( params->ui.x, params->ui.y, gbOffsetChar( params->ui.w ), gbOffsetLine( params->ui.h ), params->ui.borderColor );
	
	/* タイトル描画 */
	gbPrint(
		params->ui.x + gbOffsetChar( 1 ),
		params->ui.y + gbOffsetLine( 1 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		params->data[params->selectDataNumber].title
	);
	
	/* カレントディレクトリ表示 */
	if( params->data[params->selectDataNumber].flags & CGF_SAVE ){
		gbPrintf(
			params->ui.x + gbOffsetChar( 1 ),
			params->ui.y + gbOffsetLine( 3 ),
			params->ui.fgTextColor,
			params->ui.bgTextColor,
			"Save in: %s",
			dirhGetCwd( st_dirh )
		);
	} else{
		gbPrintf(
			params->ui.x + gbOffsetChar( 1 ),
			params->ui.y + gbOffsetLine( 3 ),
			params->ui.fgTextColor,
			params->ui.bgTextColor,
			"Look in: %s",
			dirhGetCwd( st_dirh )
		);
	}
	
	/* ファイルリストボーダー */
	gbLineRectRel(
		params->ui.x + gbOffsetChar( 1 ),
		params->ui.y + gbOffsetLine( 4 ),
		gbOffsetChar( params->ui.w - 2 ),
		gbOffsetLine( params->ui.h - 12 ),
		tempdata->focus == CMNDLG_GET_FILENAME_FOCUS_LIST ? params->ui.fcTextColor : params->ui.fgTextColor
	);
	
	gbPrint(
		params->ui.x + gbOffsetChar( 1 ),
		params->ui.y + gbOffsetLine( params->ui.h - 7 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		"Filename:"
	);
	
	/* ファイル名ボーダー */
	gbLineRectRel(
		params->ui.x + gbOffsetChar( 1 ),
		params->ui.y + gbOffsetLine( params->ui.h - 6 ),
		gbOffsetChar( params->ui.w - 2 ),
		gbOffsetLine( 3 ),
		tempdata->focus == CMNDLG_GET_FILENAME_FOCUS_NAME ? params->ui.fcTextColor : params->ui.fgTextColor
	);
	
	gbPrint(
		params->ui.x + gbOffsetChar( 3 ),
		params->ui.y + gbOffsetLine( params->ui.h - 5 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		params->data[params->selectDataNumber].name
	);
	
	gbPrint(
		params->ui.x + gbOffsetChar( 1 ),
		params->ui.y + gbOffsetLine( params->ui.h - 2 ),
		params->ui.fgTextColor,
		params->ui.bgTextColor,
		"SELECT: Usage"
	);
	
	if( st_all_entries < ( params->ui.h - 14 ) || tempdata->selected < ( ( params->ui.h - 14 ) >> 1 ) ){
		line_number = 0;
	} else if( tempdata->selected > st_all_entries - ( ( params->ui.h - 14 ) >> 1 ) ){
		line_number = st_all_entries - ( params->ui.h - 14 );
	} else{
		line_number = tempdata->selected - ( ( params->ui.h - 14 ) >> 1 );
	}
	dirhSeek( st_dirh, DW_SEEK_SET, line_number );
	
	for( rest_lines = params->ui.h - 14, drawline = 0; rest_lines; rest_lines--, line_number++, drawline++ ){
		dirent_name = dirhRead( st_dirh );
		if( ! dirent_name ) break;
		
		if( line_number == ((int *)(params->base.tempBuffer))[params->selectDataNumber] ){
			gbPrint(
				params->ui.x + gbOffsetChar( 2 ),
				params->ui.y + gbOffsetLine( 5 + drawline ),
				params->ui.fcTextColor,
				params->ui.bgTextColor,
				dirent_name
			);
		} else{
			gbPrint(
				params->ui.x + gbOffsetChar( 2 ),
				params->ui.y + gbOffsetLine( 5 + drawline ),
				line_number < dircnt ? params->extraUi.dirColor : params->ui.fgTextColor,
				params->ui.bgTextColor,
				dirent_name
			);
		}
	}
}

static bool cmndlg_get_filename_move_parentdir( char *path )
{
	char *pos = strutilCounterReversePbrk( path, "/" );
	if( pos ){
		for( ; path < pos; pos-- ){
			if( *pos == '/' ){
				*pos = '\0';
				return true;
			}
		}
	}
	return false;
}

static bool cmndlg_get_filename_message_start( const char *title, const char *message )
{
	CmndlgMessageParams *msg_params = (CmndlgMessageParams *)memsceMalloc( sizeof( CmndlgMessageParams ) );
	if( msg_params ){
		strutilSafeCopy( msg_params->title, title, 64 );
		strutilSafeCopy( msg_params->message, message, 512 );
		msg_params->options        = CMNDLG_MESSAGE_DISPLAY_CENTER;
		msg_params->rc             = 0;
		msg_params->ui.x           = 0;
		msg_params->ui.y           = 0;
		msg_params->ui.fgTextColor = st_params->ui.fgTextColor;
		msg_params->ui.fcTextColor = st_params->ui.fcTextColor;
		msg_params->ui.bgTextColor = st_params->ui.bgTextColor;
		msg_params->ui.bgColor     = 0xdd000000;
		msg_params->ui.borderColor = 0x0;
		
		if( cmndlgMessageStart( msg_params ) ){
			memsceFree( msg_params );
			return false;
		}
		return true;
	}
	return false;
}

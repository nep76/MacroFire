/*=========================================================

	mfmenu.c

	MacroFire メニュー

=========================================================*/
#include "mfmenu.h"

/*=========================================================
	ローカル定数
=========================================================*/
#define MF_MENU_MAX_NUMBER_OF_THREADS 64

#define MF_MENU_STACK_SIZE( x ) ( sizeof( void * ) * ( x ) )
#define MF_MENU_STACK_NUM       16

#define MF_MENU_SUSPEND_THREADS false
#define MF_MENU_RESUME_THREADS  true

#define MF_MENU_ALT_BUTTONS_COUNT 13
#define MF_MENU_ALT_BUTTONS \
	{ PSP_CTRL_SELECT,   "SELECT"   },\
	{ PSP_CTRL_START,    "START"    },\
	{ PSP_CTRL_UP,       "UP"       },\
	{ PSP_CTRL_RIGHT,    "RIGHT"    },\
	{ PSP_CTRL_DOWN,     "DOWN"     },\
	{ PSP_CTRL_LEFT,     "LEFT"     },\
	{ PSP_CTRL_LTRIGGER, "LTRIGGER" },\
	{ PSP_CTRL_RTRIGGER, "RTRIGGER" },\
	{ PSP_CTRL_TRIANGLE, "TRIANGLE" },\
	{ PSP_CTRL_CIRCLE,   "CIRCLE"   },\
	{ PSP_CTRL_CROSS,    "CROSS"    },\
	{ PSP_CTRL_SQUARE,   "SQUARE"   },\
	{ PSP_CTRL_HOME,     "HOME"     },\
	{ 0, NULL }


/*=========================================================
	ローカル型宣言
=========================================================*/
struct mf_frame_buffers {
	void *vrambase;
	void *origaddr;
	
	void *vram;
	void *display;
	void *draw;
	void *clear;
	
	unsigned int frameSize;
	bool volatileMem;
};

struct mf_threads {
	SceUID list[MF_MENU_MAX_NUMBER_OF_THREADS];
	int    count;
};

struct mf_menu_pos {
	MfMenuTable *last;
	unsigned short currentTable;
	unsigned short currentRow;
	unsigned short currentCol;
	MfMenuProc extra;
};

struct mf_menu_stack {
	MfProc       *memory;
	unsigned int index;
	unsigned int size;
};

struct mf_menu_input {
	SceCtrlData *pad;
	MfHprmKey   *hprmKey;
	PadctrlUID  uid;
};

struct mf_menu_pref {
	bool useVolatileMem;
	PadutilRemap *remap;
};

struct mf_menu_info {
	char text[MF_MENU_INFOTEXT_LENGTH];
	unsigned int options;
};

struct mf_menu_params {
	bool screenUpdate;
	bool noqq;
	bool forcedQuit;
	MfDialogType lastDialogType;
	struct mf_frame_buffers frameBuffer;
	struct mf_threads       threads;
	struct mf_menu_pos      menu;
	struct mf_menu_input    ctrl;
	struct mf_menu_stack    stack;
	struct mf_menu_info     info;
	unsigned int waitMicroSecForUpdate;
	MfFuncMenu proc, nextproc;
};

enum mf_menu_stack_ctrl {
	MF_MENU_STACK_CREATE,
	MF_MENU_STACK_PUSH,
	MF_MENU_STACK_POP,
	MF_MENU_STACK_DESTROY
};

enum mf_menu_move_directions {
	MF_MENU_MOVE_UP,
	MF_MENU_MOVE_RIGHT,
	MF_MENU_MOVE_DOWN,
	MF_MENU_MOVE_LEFT
};
	
struct mf_main_pref {
	bool engine;
	PadutilButtons menu;
	PadutilButtons toggle;
	bool statusNotification;
};

/*=========================================================
	ローカル関数
=========================================================*/
#ifdef DEBUG_ENABLED
static void mf_debug( void );
#endif
static bool mf_menu_get_target_threads( SceUID *thread_id_list, int *thread_id_count );
static void mf_menu_control_thread_stat( bool stat, SceUID *thread_id_list, int thread_id_count );
static bool mf_alloc_buffers( struct mf_frame_buffers *fb );
static void mf_free_buffers( struct mf_frame_buffers *fb );
static void mf_draw_frame( void );
static void mf_menu_init_tables( MfMenuTable *menu, unsigned short menucnt );
static void mf_menu_draw_tables( MfMenuTable *menu, unsigned short menucnt, struct mf_menu_pos *pos );
static bool mf_menu_move_table( enum mf_menu_move_directions dir, MfMenuTable *menu, unsigned short menucnt, struct mf_menu_pos *pos );
static bool mf_menu_nearest_table( unsigned int pri, unsigned int sec, unsigned int *save_pri, unsigned int *save_sec );
static bool mf_menu_farest_table( unsigned int pri, unsigned int sec, unsigned int *save_pri, unsigned int *save_sec );
static bool mf_menu_select_table_vertical( MfMenuTable *menu, unsigned short targ_tid, struct mf_menu_pos *pos, bool reverse );
static bool mf_menu_select_table_horizontal( MfMenuTable *menu, unsigned short targ_tid, struct mf_menu_pos *pos, bool reverse );
static bool mf_menu_stack( struct mf_menu_stack *stack, enum mf_menu_stack_ctrl ctrl, void *menu );
static unsigned int mf_label_width( char *str );
static void mf_root_menu( MfMenuMessage message );
static void mf_menu_audio( bool enable );
static void mf_indicator( void );

/*=========================================================
	ローカル変数
=========================================================*/
static SceUID st_thread_id_list_first[MF_MENU_MAX_NUMBER_OF_THREADS];
static int    st_thread_id_count_first;

static struct mf_menu_pref   st_pref;
static struct mf_menu_params *st_params;

static PadutilButtonName *st_symbols;

#ifdef DEBUG_ENABLED
static void mf_debug( void )
{
	gbPrintf( gbOffsetChar( 5 ), gbOffsetLine( 2 ), 0xff00ffff, MF_COLOR_TEXT_BG,
		"T:%d M:%d ( KH:%d U:%d KL:%d V:%d )",
		sceKernelTotalFreeMemSize(),
		sceKernelMaxFreeMemSize(),
		sceKernelPartitionTotalFreeMemSize( 1 ),
		sceKernelPartitionTotalFreeMemSize( 2 ),
		sceKernelPartitionTotalFreeMemSize( 4 ),
		sceKernelPartitionTotalFreeMemSize( 5 )
	);
	gbPrintf( gbOffsetChar( 1 ), gbOffsetLine( 0 ), 0xff0000ff, MF_COLOR_TEXT_BG, "Development build - %s", __DATE__ );
}
#endif

/*-----------------------------------------------
	メニューの初期化。
	
	メニューを呼び出した際に他スレッドを停止させる為に使用するスレッドリストを取得。
	このスレッドリストに入っているスレッドは、メニュー起動時に停止しない。
-----------------------------------------------*/
bool mfMenuInit( void )
{
	if( sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, st_thread_id_list_first, MF_MENU_MAX_NUMBER_OF_THREADS, &st_thread_id_count_first ) < 0 ){
		dbgprint( "Failed to get a first thread-id list" );
		return false;
	}
	sceCtrlSetSamplingMode( PSP_CTRL_MODE_ANALOG );
	
	/* 設定初期化 */
	st_pref.useVolatileMem = false;
	st_pref.remap          = NULL;
	
	return true;
}

void mfMenuDestroy( void )
{
	if( st_pref.remap ) padutilDestroyRemapArray( st_pref.remap );
}

void mfMenuIniLoad( IniUID ini, char *buf, size_t len )
{
	unsigned short i, j;
	PadutilButtonName button_names[] = { MF_MENU_ALT_BUTTONS };
	
	inimgrGetBool( ini, "Menu", "UseVolatileMemory", &(st_pref.useVolatileMem) );
	
	for( i = 0, j = 0; i < MF_MENU_ALT_BUTTONS_COUNT; i++ ){
		if( inimgrGetString( ini, "AlternativeButtons", button_names[i].name, buf, len ) > 0 ){
			if( ! st_pref.remap ) st_pref.remap = padutilCreateRemapArray( MF_MENU_ALT_BUTTONS_COUNT );
			st_pref.remap[j].realButtons = mfConvertButtonN2C( buf );
			st_pref.remap[j].remapButtons  = button_names[i].button;
		}
	}
}

void mfMenuIniSave( IniUID ini, char *buf, size_t len )
{
	unsigned short i;
	PadutilButtonName button_names[] = { MF_MENU_ALT_BUTTONS };
	
	inimgrSetBool( ini, "Menu", "UseVolatileMemory", false );
	
	for( i = 0; i < MF_MENU_ALT_BUTTONS_COUNT; i++ ){
		strlwr( button_names[i].name );
		button_names[i].name[0] = toupper( button_names[i].name[0] );
		
		inimgrSetString( ini, "AlternativeButtons", button_names[i].name, "" );
	}
}

void mfMenuQuit( void )
{
	if( ! st_params ) return;
	
	st_params->forcedQuit = true;
}

void mfMenuWait( unsigned int microsec )
{
	if( ! st_params ) return;
	
	st_params->waitMicroSecForUpdate = microsec;
}

void mfMenuProc( MfMenuProc proc )
{
	if( ! st_params ) return;
	
	st_params->nextproc = proc;
}

bool mfMenuSetExtra( MfMenuProc extra )
{
	if( ! st_params || st_params->menu.extra ){
		return false;
	}
	
	st_params->menu.extra = extra;
	( st_params->menu.extra )( MF_MM_INIT );
	
	return true;
}

void mfMenuExitExtra( void )
{
	if( ! st_params ) return;
	
	( st_params->menu.extra )( MF_MM_TERM );
	st_params->menu.extra = NULL;
	
	mfMenuResetKeyRepeat();
}

void mfMenuEnableQuickQuit( void )
{
	if( ! st_params ) return;
	
	st_params->noqq--;
	if( st_params->noqq < 0 ) st_params->noqq = 0;
	dbgprintf( "Decreased menu-noqq count: %d", st_params->noqq );
}

void mfMenuDisableQuickQuit( void )
{
	if( ! st_params ) return;
	
	st_params->noqq++;
	dbgprintf( "Increased menu-noqq count: %d", st_params->noqq );
}

char *mfMenuButtonsSymbolList( PadutilButtons buttons, char *str, size_t len )
{
	padutilGetButtonNamesByCode( st_symbols, buttons, " ", 0, str, len );
	return str;
}

bool mfMenuDrawDialog( MfDialogType dialog, bool update )
{
	switch( dialog ){
		case MFDIALOG_MESSAGE:
			if( update   ) update = mfDialogMessageDraw();
			if( ! update ) mfDialogMessageResult();
			return true;
		case MFDIALOG_SOSK:
			if( update   ) update = mfDialogSoskDraw();
			if( ! update ) mfDialogSoskResult();
			return true;
		case MFDIALOG_NUMEDIT:
			if( update   ) update = mfDialogNumeditDraw();
			if( ! update ) mfDialogNumeditResult();
			return true;
		case MFDIALOG_GETFILENAME:
			if( update   ) update = mfDialogGetfilenameDraw();
			if( ! update ) mfDialogGetfilenameResult();
			return true;
		case MFDIALOG_DETECTBUTTONS:
			if( update   ) update = mfDialogDetectbuttonsDraw();
			if( ! update ) mfDialogDetectbuttonsResult();
			return true;
		default:
			return false;
	}
}

void mfMenuInitTables( MfMenuTable menu[], unsigned short menucnt )
{
	if( ! st_params ) return;
	
	dbgprint( "Initializing next menu entries" );
	st_params->menu.currentTable = 0;
	st_params->menu.currentRow   = 0;
	st_params->menu.currentCol   = 0;
	st_params->menu.last         = menu;
	
	mf_menu_init_tables( menu, menucnt );
}

bool mfMenuDrawTables( MfMenuTable menu[], unsigned short menucnt, unsigned int opt )
{
	struct mf_menu_pos dupe_pos;
	bool refind = true;
	MfDialogType dialog;
	MfCtrlMessage message;
	
	if( ! st_params ) return false;
	
	/* メニュースタック用 */
	if( st_params->menu.last != menu ) mfMenuInitTables( menu, menucnt );
	
	/* メニューラベルを取得し描画 */
	mf_menu_draw_tables( menu, menucnt, &(st_params->menu) );
	
	if( st_params->menu.extra ){
		/* 拡張関数が設定されていれば実行 */
		( st_params->menu.extra )( MF_MM_PROC );
		message = MF_CM_EXTRA;
	} else if( ! ( opt & MF_MENU_DISPLAY_ONLY ) ){
		/* ダイアログの確認 */
		dialog = mfDialogCurrentType();
		
		/* メッセージを確定 */
		if( dialog != MFDIALOG_NONE ){
			if( st_params->lastDialogType != dialog ){
				message = MF_CM_DIALOG_START;
				st_params->lastDialogType = dialog;
			} else{
				message = MF_CM_DIALOG_UPDATE;
			}
		} else if( st_params->lastDialogType != dialog ){
			message = MF_CM_DIALOG_FINISH;
			st_params->lastDialogType = dialog;
			mfMenuResetKeyRepeat();
		} else{
			message = MF_CM_PROC;
		}
		
		/* ダイアログを描画 */
		mfMenuDrawDialog( dialog, true );
	} else{
		message = MF_CM_PROC;
	}
	
	/* ダイアログメッセージの場合は先行処理 */
	if( message != MF_CM_PROC ){
		if( message != MF_CM_EXTRA && menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].ctrl ){
			if( ! ( menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].ctrl )(
				message,
				menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].label,
				menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].var,
				menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].arg,
				NULL
			) ) return false;
		}
		if( message == MF_CM_DIALOG_FINISH ){
			message = MF_CM_PROC;
		} else{
			return true;
		}
	}
	
	/* BACKの検出 */
	if( mfMenuIsPressed( PSP_CTRL_CROSS ) ) return false;
	
	/* オプション処理 */
	if( ( opt & MF_MENU_DISPLAY_ONLY ) || message == MF_CM_EXTRA ) return true;
	
	/* 情報バーのデータを取得 */
	{
		unsigned int opt = menu[st_params->menu.currentTable].cols == 1 ? 0 : MF_MENU_INFOTEXT_MULTICOLUMN_CTRL;
		
		if( menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].active ){
			( menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].ctrl )(
				MF_CM_INFO,
				menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].label,
				menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].var,
				menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].arg,
				&opt
			);
		}
		if( st_params->info.text[0] == '\0' ) mfMenuSetInfoText( MF_MENU_INFOTEXT_COMMON_CTRL, "" );
	}
	
	/* 現在の位置をバックアップ */
	dupe_pos = st_params->menu;
	
	/* 操作 */
	while( refind ){
		refind = false;
		
		if( mfMenuIsPressed( PSP_CTRL_UP ) ){
			if( st_params->menu.currentRow ){
				st_params->menu.currentRow--;
			} else if( ! mf_menu_move_table( MF_MENU_MOVE_UP, menu, menucnt, &(st_params->menu) ) ){
				refind = true;
				break;
			}
		} else if( mfMenuIsPressed( PSP_CTRL_DOWN ) ){
			if( st_params->menu.currentRow < menu[st_params->menu.currentTable].rows - 1 ){
				st_params->menu.currentRow++;
			} else if( ! mf_menu_move_table( MF_MENU_MOVE_DOWN, menu, menucnt, &(st_params->menu) ) ){
				refind = true;
				break;
			}
		} else if( mfMenuIsPressed( PSP_CTRL_LEFT ) ){
			if( st_params->menu.currentCol ){
				st_params->menu.currentCol--;
			} else if( ! mf_menu_move_table( MF_MENU_MOVE_LEFT, menu, menucnt, &(st_params->menu) ) ){
				refind = true;
				break;
			}
		} else if( mfMenuIsPressed( PSP_CTRL_RIGHT ) ){
			if( st_params->menu.currentCol < menu[st_params->menu.currentTable].cols - 1 ){
				st_params->menu.currentCol++;
			} else if( ! mf_menu_move_table( MF_MENU_MOVE_RIGHT, menu, menucnt, &(st_params->menu) ) ){
				refind = true;
				break;
			}
		}
		
		if( ! menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].active ){
			short pos_pos, pos_neg;
			if( mfMenuIsAnyPressed( PSP_CTRL_UP | PSP_CTRL_DOWN ) ){
				refind = true;
				pos_pos = st_params->menu.currentCol + 1;
				pos_neg = st_params->menu.currentCol - 1;
				while( refind && ( pos_pos >= 0 || pos_neg >= 0 ) ){
					if( pos_pos >= menu[st_params->menu.currentTable].cols ) pos_pos = -1;
					
					if( pos_neg >= 0 ){
						if( menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][pos_neg].active ){
							st_params->menu.currentCol = pos_neg;
							refind = false;
						}
						pos_neg--;
					}else if( pos_pos >= 0 ){
						if( menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][pos_pos].active ){
							st_params->menu.currentCol = pos_pos;
							refind = false;
						}
						pos_pos++;
					}
				}
			} else if( mfMenuIsAnyPressed( PSP_CTRL_RIGHT | PSP_CTRL_LEFT ) ){
				refind = true;
				pos_pos = st_params->menu.currentRow + 1;
				pos_neg = st_params->menu.currentRow - 1;
				while( refind && ( pos_pos >= 0 || pos_neg >= 0 ) ){
					if( pos_pos >= menu[st_params->menu.currentTable].rows ) pos_pos = -1;
					
					if( pos_neg >= 0 ){
						if( menu[st_params->menu.currentTable].entry[pos_neg][st_params->menu.currentCol].active ){
							st_params->menu.currentRow = pos_neg;
							refind = false;
						}
						pos_neg--;
					}else if( pos_pos >= 0 ){
						if( menu[st_params->menu.currentTable].entry[pos_pos][st_params->menu.currentCol].active ){
							st_params->menu.currentRow = pos_pos;
							refind = false;
						}
						pos_pos++;
					}
				}
			}
		}
		
		if( menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].active ){
			if( ! ( menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].ctrl )(
				message,
				menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].label,
				menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].var,
				menu[st_params->menu.currentTable].entry[st_params->menu.currentRow][st_params->menu.currentCol].arg,
				NULL
			) ) return false;
		}
	}
	
	if( refind ) st_params->menu = dupe_pos;
	
	return true;
}

inline SceCtrlData *mfMenuGetCurrentPadData( void )
{
	if( ! st_params ) return 0;
	
	return st_params->ctrl.pad;
}

inline unsigned int mfMenuGetCurrentButtons( void )
{
	if( ! st_params ) return 0;
	
	return st_params->ctrl.pad->Buttons;
}

inline unsigned char mfMenuGetCurrentAnalogStickX( void )
{
	if( ! st_params ) return 0;
	
	return st_params->ctrl.pad->Lx;
}

inline unsigned char mfMenuGetCurrentAnalogStickY( void )
{
	if( ! st_params ) return 0;
	
	return st_params->ctrl.pad->Ly;
}

inline MfHprmKey mfMenuGetCurrentHprmKey( void )
{
	if( ! st_params ) return 0;
	
	return *(st_params->ctrl.hprmKey);
}

inline void mfMenuResetKeyRepeat( void )
{
	if( ! st_params ) return;
	
	padctrlResetRepeat( st_params->ctrl.uid );
}

unsigned int mfMenuScroll( int selected, unsigned int viewlines, unsigned int maxlines )
{
	int scroll_lines, line_number;
	
	for( scroll_lines = selected - ( viewlines >> 1 ), line_number = 0; scroll_lines > 0; scroll_lines--, line_number++ ){
		if( maxlines - line_number < viewlines ){
			break;
		}
	}
	
	return line_number;
}

void mfMenuSetTablePosition( MfMenuTable *menu, unsigned short tableid, int x, int y )
{
	menu[--tableid].x = x;
	menu[tableid].y = y;
}

void mfMenuSetColumnWidth( MfMenuTable *menu, unsigned short tableid, unsigned short col, unsigned int width )
{
	if( ! menu[--tableid].colsWidth ){
		dbgprintf( "Table %d of menu(%p) is not multi-columns", tableid, menu );
		return;
	}
	menu[tableid].colsWidth[--col] = width;
}

void mfMenuSetTableLabel( MfMenuTable *menu, unsigned short tableid, char *label )
{
	menu[--tableid].label = label;
}

void mfMenuSetTableEntry( MfMenuTable *menu, unsigned short tableid, unsigned short row, unsigned short col, char *label, MfControl ctrl, void *var, void *arg )
{
	menu[--tableid].entry[--row][--col].active = ctrl ? true : false;
	menu[tableid].entry[row][col].ctrl           = ctrl;
	menu[tableid].entry[row][col].label          = label;
	menu[tableid].entry[row][col].var            = var;
	menu[tableid].entry[row][col].arg            = arg;
}

void mfMenuActiveTableEntry( MfMenuTable *menu, unsigned short tableid, unsigned short row, unsigned short col )
{
	tableid--;
	row--;
	col--;
	menu[tableid].entry[row][col].active = menu[tableid].entry[row][col].ctrl ? true : false;
}

void mfMenuInactiveTableEntry( MfMenuTable *menu, unsigned short tableid, unsigned short row, unsigned short col )
{
	menu[--tableid].entry[--row][--col].active = false;
}

static bool mf_menu_move_table( enum mf_menu_move_directions dir, MfMenuTable *menu, unsigned short menucnt, struct mf_menu_pos *pos )
{
	unsigned short i;
	unsigned short target_table = pos->currentTable;
	unsigned int   save_diff_x  = ~0;
	unsigned int   save_diff_y  = ~0;
	
	/* 入力方向にもっとも近いテーブルを探す */
	
	dbgprint( "Out of menu-range. find a next table..." );
	switch( dir ){
		case MF_MENU_MOVE_UP:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->currentTable].y <= menu[i].y ) continue;
				if( mf_menu_nearest_table( abs( menu[pos->currentTable].y - menu[i].y ), abs( menu[pos->currentTable].x - menu[i].x ), &save_diff_y, &save_diff_x ) ){
					target_table = i;
				}
			}
			if( target_table != pos->currentTable ){
				return mf_menu_select_table_vertical( menu, target_table, pos, false );
			} else{
				break;
			}
		case MF_MENU_MOVE_RIGHT:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->currentTable].x >= menu[i].x ) continue;
				if( mf_menu_nearest_table( abs( menu[pos->currentTable].x - menu[i].x ), abs( menu[pos->currentTable].y - menu[i].y ), &save_diff_x, &save_diff_y ) ){
					target_table = i;
				}
			}
			if( target_table != pos->currentTable ){
				return mf_menu_select_table_horizontal( menu, target_table, pos, false );
			} else{
				break;
			}
		case MF_MENU_MOVE_DOWN:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->currentTable].y >= menu[i].y ) continue;
				if( mf_menu_nearest_table( abs( menu[pos->currentTable].y - menu[i].y ), abs( menu[pos->currentTable].x - menu[i].x ), &save_diff_y, &save_diff_x ) ){
					target_table = i;
				}
			}
			if( target_table != pos->currentTable ){
				return mf_menu_select_table_vertical( menu, target_table, pos, false );
			} else{
				break;
			}
		case MF_MENU_MOVE_LEFT:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->currentTable].x <= menu[i].x ) continue;
				if( mf_menu_nearest_table( abs( menu[pos->currentTable].x - menu[i].x ), abs( menu[pos->currentTable].y - menu[i].y ), &save_diff_x, &save_diff_y ) ){
					target_table = i;
				}
			}
			if( target_table != pos->currentTable ){
				return mf_menu_select_table_horizontal( menu, target_table, pos, false );
			} else{
				break;
			}
	}
	
	/* 見つからなければ、入力方向と逆方向のもっとも遠いテーブルを探す */
	dbgprint( "Next table not found. position wrap." );
	save_diff_x  = ~0;
	save_diff_y  = ~0;
	switch( dir ){
		case MF_MENU_MOVE_UP:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->currentTable].y >= menu[i].y ) continue;
				if( mf_menu_farest_table( abs( menu[pos->currentTable].y - menu[i].y ), abs( menu[pos->currentTable].x - menu[i].x ), &save_diff_y, &save_diff_x ) ){
					target_table = i;
				}
			}
			return mf_menu_select_table_vertical( menu, target_table, pos, true );
		case MF_MENU_MOVE_RIGHT:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->currentTable].x <= menu[i].x ) continue;
				if( mf_menu_farest_table( abs( menu[pos->currentTable].x - menu[i].x ), abs( menu[pos->currentTable].y - menu[i].y ), &save_diff_x, &save_diff_y ) ){
					target_table = i;
				}
			}
			return mf_menu_select_table_horizontal( menu, target_table, pos, true );
		case MF_MENU_MOVE_DOWN:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->currentTable].y <= menu[i].y ) continue;
				if( mf_menu_farest_table( abs( menu[pos->currentTable].y - menu[i].y ), abs( menu[pos->currentTable].x - menu[i].x ), &save_diff_y, &save_diff_x ) ){
					target_table = i;
				}
			}
			return mf_menu_select_table_vertical( menu, target_table, pos, true );
		case MF_MENU_MOVE_LEFT:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->currentTable].x >= menu[i].x ) continue;
				if( mf_menu_farest_table( abs( menu[pos->currentTable].x - menu[i].x ), abs( menu[pos->currentTable].y - menu[i].y ), &save_diff_x, &save_diff_y ) ){
					target_table = i;
				}
			}
			return mf_menu_select_table_horizontal( menu, target_table, pos, true );
	}
	
	return false;
}

static bool mf_menu_nearest_table( unsigned int pri, unsigned int sec, unsigned int *save_pri, unsigned int *save_sec )
{
	/*
		*save_pri, *save_sec は、全ビット1(~0)の場合のみ、特別に未定義値として扱う。
		従って、*save_pri, *save_sec に渡す変数はかならず ~0 で初期化した状態で渡す。
	*/
	
	if( *save_pri == ~0 || pri < *save_pri ){
		*save_pri = pri;
		return true;
	} else if( *save_sec == sec ){
		if( ! *save_sec || sec < *save_sec ){
			*save_sec = sec;
			return true;
		}
	}
	return false;
}

static bool mf_menu_farest_table( unsigned int pri, unsigned int sec, unsigned int *save_pri, unsigned int *save_sec )
{
	/*
		*save_pri, *save_sec は、全ビット1(~0)の場合のみ、特別に未定義値として扱う。
		従って、*save_pri, *save_sec に渡す変数はかならず ~0 で初期化した状態で渡す。
	*/
	
	if( *save_pri == ~0 || pri > *save_pri ){
		*save_pri = pri;
		return true;
	} else if( *save_sec == sec ){
		if( ! *save_sec || sec > *save_sec ){
			*save_sec = sec;
			return true;
		}
	}
	return false;
}

static bool mf_menu_select_table_horizontal( MfMenuTable *menu, unsigned short targ_tid, struct mf_menu_pos *pos, bool reverse )
{
	if( pos->currentTable == targ_tid ){
		dbgprintf( "Selected same table id: Col %d, %d", pos->currentCol, menu[targ_tid].cols );
		pos->currentCol = pos->currentCol ? 0 : menu[targ_tid].cols - 1;
	} else{
		unsigned short i, targ_row = 0, targ_col = 0;
		unsigned int current_y = menu[pos->currentTable].y + gbOffsetLine( pos->currentRow );
		unsigned int diff_y, last_diff_y = ~0;
		
		for( i = 0; i < menu[targ_tid].rows; i++ ){
			diff_y = abs( current_y - ( menu[targ_tid].y + gbOffsetLine( i ) ) );
			if( last_diff_y == ~0 || last_diff_y > diff_y ){
				last_diff_y = diff_y;
				targ_row = i;
			}
		}
		
		if( menu[pos->currentTable].x > menu[targ_tid].x ){
			targ_col = reverse ? 0 : menu[targ_tid].cols - 1;
		} else{
			targ_col = reverse ? menu[targ_tid].cols - 1 : 0;
		}
		
		pos->currentTable = targ_tid;;
		pos->currentRow   = targ_row;
		pos->currentCol   = targ_col;
	}
	
	return true;
}

static bool mf_menu_select_table_vertical( MfMenuTable *menu, unsigned short targ_tid, struct mf_menu_pos *pos, bool reverse )
{
	
	if( pos->currentTable == targ_tid ){
		dbgprintf( "Selected same table id: Row %d, %d", pos->currentRow, menu[targ_tid].rows );
		pos->currentRow = pos->currentRow ? 0 : menu[targ_tid].rows - 1;
	} else{
		unsigned short i, targ_row = 0, targ_col = 0;
		unsigned int current_x = menu[pos->currentTable].x;
		unsigned int comp_x, diff_x, last_diff_x = ~0;

		if( menu[pos->currentTable].colsWidth ){
			for( i = 0; i < pos->currentCol; i++ ){
				current_x += menu[pos->currentTable].colsWidth[i];
			}
		}
		
		comp_x = menu[targ_tid].x;
		for( i = 0; i < menu[targ_tid].cols; i++ ){
			diff_x = abs( current_x - comp_x );
			if( last_diff_x == ~0 || last_diff_x > diff_x ){
				last_diff_x = diff_x;
				targ_col = i;
			}
			if( menu[targ_tid].colsWidth ) comp_x += menu[targ_tid].colsWidth[i];
		}
		
		if( menu[pos->currentTable].y > menu[targ_tid].y ){
			targ_row = reverse ? 0 : menu[targ_tid].rows - 1;
		} else{
			targ_row = reverse ? menu[targ_tid].rows - 1 : 0;
		}
		
		pos->currentTable = targ_tid;
		pos->currentRow   = targ_row;
		pos->currentCol   = targ_col;
	}
	return true;
}

static unsigned int mf_label_width( char *str )
{
	unsigned int max_width = 0;
	unsigned int cur_width;
	char *lnstart, *lnend;
	
	if( ! str ) return 0;
	
	lnstart = str;
	while( ( lnend = strchr( str, '\n' ) ) ){
		cur_width = lnend - lnstart;
		if( cur_width > max_width ) max_width = cur_width;
		
		lnstart = lnend++;
	}
	
	return max_width ? max_width : strlen( str );
}

static void mf_menu_init_tables( MfMenuTable *menu, unsigned short menucnt )
{
	unsigned short i, row, col;
	
	dbgprint( "Sending message MF_CM_INIT to menu items..." );
	for( i = 0; i < menucnt; i++ ){
		for( row = 0; row < menu[i].rows; row++ ){
			for( col = 0; col < menu[i].cols; col++ ){
				dbgprintf( "Init \"%s\" (%d,%d,%d)", menu[i].entry[row][col].label, i, row, col );
				if( menu[i].entry[row][col].ctrl ) ( menu[i].entry[row][col].ctrl )(
					MF_CM_INIT,
					menu[i].entry[row][col].label,
					menu[i].entry[row][col].var,
					menu[i].entry[row][col].arg,
					NULL
				);
			}
		}
	}
	dbgprint( "MF_CM_INIT completed" );
}

static void mf_menu_draw_tables( MfMenuTable *menu, unsigned short menucnt, struct mf_menu_pos *pos )
{
	unsigned short i, row, col;
	unsigned int *col_width, width, col_offset, row_offset;
	char labelbuf[MF_CTRL_BUFFER_LENGTH];
	
	for( i = 0; i < menucnt; i++ ){
		if( menu[i].label ){
			col_offset = gbOffsetChar( 2 );
			row_offset = gbOffsetLine( 1 ) + 2;
			gbPrint( menu[i].x, menu[i].y,  MF_COLOR_TEXT_LABEL, MF_COLOR_TEXT_BG, menu[i].label );
			gbLineRel( menu[i].x, menu[i].y + gbOffsetLine( 1 ), gbOffsetChar( 40 ), 0, MF_COLOR_TEXT_LABEL );
		} else{
			col_offset = 0;
			row_offset = 0;
		}
		for( col = 0; col < menu[i].cols; col++ ){
			col_width = ( menu[i].colsWidth && ( menu[i].colsWidth[col] == 0 ) ) ? &(menu[i].colsWidth[col]) : NULL;
			if( menu[i].colsWidth && col ) col_offset += gbOffsetChar( menu[i].colsWidth[col - 1] + 1 );
			for( row = 0; row < menu[i].rows; row++ ){
				labelbuf[0] = '\0';
				if( menu[i].entry[row][col].ctrl ){
					( menu[i].entry[row][col].ctrl )(
						MF_CM_LABEL,
						menu[i].entry[row][col].label,
						menu[i].entry[row][col].var,
						menu[i].entry[row][col].arg,
						labelbuf
					);
				}
				if( labelbuf[0] == '\0' ) strutilCopy( labelbuf, menu[i].entry[row][col].label, sizeof( labelbuf ) );
				
				if( labelbuf[0] != '\0' ){
					gbPrint(
						menu[i].x + col_offset,
						menu[i].y + row_offset + gbOffsetLine( row ),
						(
							i == pos->currentTable && row == pos->currentRow && col == pos->currentCol ? MF_COLOR_TEXT_FC :
							menu[i].entry[row][col].active ? MF_COLOR_TEXT_FG :
							                                 MF_COLOR_TEXT_INACT
						),
						MF_COLOR_TEXT_BG,
						labelbuf
					);
					
					if( col_width ){
						width = mf_label_width( labelbuf );
						if( *col_width < width ) *col_width = width;
					}
				}
			}
		}
	}
}

static void mf_root_menu( MfMenuMessage message )
{
	static MfMenuTable *menu;
	static HeapUID pheap;
	static struct mf_main_pref *main_pref;
	
	switch( message ){
		case MF_MM_INIT:
			dbgprint( "Creating main-menu table" );
			menu = mfMenuCreateTables(
				3,
				1, 1,
				4, 1,
				gMftabEntryCount, 1
			);
			pheap = heapCreate( sizeof( struct mf_main_pref ) + sizeof( MfCtrlDefGetButtonsPref ) + HEAP_HEADER_SIZE * 2 );
			if( ! menu || ! pheap ){
				if( menu  ) mfMenuDestroyTables( menu );
				if( pheap ) heapDestroy( pheap );
				return;
			}
			
			{
				unsigned short row;
				void *menuproc;
				
				MfCtrlDefGetButtonsPref *hotkey_pref = heapCalloc( pheap, sizeof( MfCtrlDefGetButtonsPref ) );
				main_pref = heapCalloc( pheap, sizeof( struct mf_main_pref ) );
				main_pref->engine      = mfIsEnabled();
				main_pref->menu        = mfGetMenuButtons();
				main_pref->toggle      = mfGetToggleButtons();
				main_pref->statusNotification = mfOverlayMessageIsRunning();
				
				hotkey_pref->availButtons = MF_HOTKEY_BUTTONS;
				
				mfMenuSetTablePosition( menu, 1, gbOffsetChar( 5 ), gbOffsetLine( 4 ) + 2 );
				mfMenuSetTableEntry( menu, 1, 1, 1, "MacroFire Engine", mfCtrlDefBool, &(main_pref->engine), NULL );
				
				mfMenuSetTablePosition( menu, 2, gbOffsetChar( 5 ), gbOffsetLine( 6 ) + 2 );
				mfMenuSetTableLabel( menu, 2, "MacroFire preference" );
				mfMenuSetTableEntry( menu, 2, 1, 1, "Buttons to launch the menu", mfCtrlDefGetButtons, &(main_pref->menu), hotkey_pref );
				mfMenuSetTableEntry( menu, 2, 2, 1, "Buttons to toggle the engine state", mfCtrlDefGetButtons, &(main_pref->toggle), hotkey_pref );
				mfMenuSetTableEntry( menu, 2, 3, 1, "Engine status notification", mfCtrlDefBool, &(main_pref->statusNotification), NULL );
				mfMenuSetTableEntry( menu, 2, 4, 1, "Analog stick sensitivity adjustment", mfCtrlDefCallback, mfAnalogStickProc( MF_MS_MENU ), NULL );
				
				mfMenuSetTablePosition( menu, 3, gbOffsetChar( 5 ), gbOffsetLine( 12 ) + 2 );
				mfMenuSetTableLabel( menu, 3, "Functions" );
				for( row = 0; row < gMftabEntryCount; row++ ){
					if( gMftab[row].proc && ( menuproc = ( gMftab[row].proc )( MF_MS_MENU ) ) ){
						mfMenuSetTableEntry( menu, 3, row + 1, 1, gMftab[row].name, mfCtrlDefCallback, menuproc, NULL );
					} else{
						mfMenuSetTableEntry( menu, 3, row + 1, 1, gMftab[row].name, NULL, NULL, NULL );
					}
				}
			}
			mfMenuInitTables( menu, 3 );
			break;
		case MF_MM_TERM:
			main_pref->engine ? mfEnable() : mfDisable();
			mfSetMenuButtons( main_pref->menu );
			mfSetToggleButtons( main_pref->toggle );
			
			if( mfOverlayMessageIsRunning() && ! main_pref->statusNotification ){
				mfOverlayMessageExit();
			} else if( ! mfOverlayMessageIsRunning() && main_pref->statusNotification ){
				mfOverlayMessageStart();
			}
			
			mfMenuDestroyTables( menu );
			heapDestroy( pheap );
			return;
		default:
			if( ! mfMenuDrawTables( menu, 3, MF_MENU_NO_OPTIONS ) ) mfMenuProc( NULL );
	}
}

static void mf_menu_audio( bool enable )
{
	unsigned int k1 = pspSdkSetK1( 0 );
	
	if( enable ){
		sceSysregAudioIoEnable();
	} else{
		sceSysregAudioIoDisable();
	}
	
	pspSdkSetK1( k1 );
}

/*-----------------------------------------------
	メニュー メインループ
-----------------------------------------------*/
void mfMenuMain( SceCtrlData *pad, MfHprmKey *hk )
{
	bool hooked =  mfIsEnabled();
	
	dbgprintf( "Memory block %d leaked", memoryGetAllocCount() );
	
	/* ユーザメモリからメニュー用の作業域を確保 */
	st_params = memoryAlloc( sizeof( struct mf_menu_params ) );
	dbgprintf( "Allocating memory for menu: %p: %d bytes", st_params, sizeof( struct mf_menu_params ) );
	if( ! st_params ){
		dbgprint( "Not enough available memory for menu" );
		return;
	}
	memset( st_params, 0, sizeof( struct mf_menu_params ) );
	st_params->screenUpdate = true;
	
	/* サスペンド/リジュームするスレッドの一覧を作成し、サスペンドする */
	sceDisplayWaitVblankStart();
	if( ! mf_menu_get_target_threads( st_params->threads.list, &(st_params->threads.count) ) ) goto DESTROY;
	mf_menu_control_thread_stat( MF_MENU_SUSPEND_THREADS, st_params->threads.list, st_params->threads.count );
	
	/* サウンドをミュートにする */
	mf_menu_audio( false );
	
	/* APIがフックされている場合、一時的に元に戻す */
	if( hooked ) mfUnhook();
	
	/* メニュースタックを確保 */
	if( ! mf_menu_stack( &(st_params->stack), MF_MENU_STACK_CREATE, NULL ) ) goto DESTROY;
	
	/* ボタンシンボルリスト生成 */
	st_symbols = padutilCreateButtonSymbols();
	if( ! st_symbols ) goto DESTROY;
	
	/* gbライブラリ初期化 */
	dbgprint( "Initializing gb..." );
	if( gbBind() < 0 ) goto DESTROY;
	gbSetOpt( GB_AUTOFLUSH | GB_ALPHABLEND | GB_AUTO_LINEBREAK );
	gbUse8BitFont( GB_FONT_PSPSYM );
	gbPrepare();
	
	/* フレームバッファとして使用するメモリを確保 */
	dbgprint( "Allocating memory for frame buffers..." );
	if( ! mf_alloc_buffers( &(st_params->frameBuffer) ) ) goto DESTROY;
	
	/* クリア用背景作成 */
	if( st_params->frameBuffer.clear ){
		/* 描画準備 */
		gbSetDisplayBuf( st_params->frameBuffer.clear );
		gbSetDrawBuf( NULL );
		gbPrepare();
		
		/* 現在表示されている画面をクリア用背景にコピー */
		sceDisplayWaitVblankStart();
		sceDmacMemcpy( st_params->frameBuffer.clear, st_params->frameBuffer.origaddr, gbGetDataFrameSize() );
		
		gbFillRect( 0, 0, SCR_WIDTH, SCR_HEIGHT, MF_COLOR_BG );
		mf_draw_frame();
	}
	gbSetDisplayBuf( st_params->frameBuffer.display );
	gbSetDrawBuf( st_params->frameBuffer.draw );
	
	/* 設定したフレームバッファで描画準備 */
	gbPrepare();
	
	/* ダイアログ初期化 */
	mfDialogInit( st_pref.remap );
	st_params->lastDialogType = mfDialogCurrentType();
	
	/* コントロール */
	st_params->ctrl.pad     = pad;
	st_params->ctrl.hprmKey = hk;
	st_params->ctrl.uid     = padctrlNew();
	if( st_params->ctrl.uid < 0 ){
		dbgprintf( "Failed to get a PadctrlUID: %x", st_params->ctrl.uid );
		st_params->ctrl.uid = 0;
		goto DESTROY;
	}
	padctrlSetRepeatButtons( st_params->ctrl.uid,
		PSP_CTRL_UP | PSP_CTRL_RIGHT | PSP_CTRL_DOWN | PSP_CTRL_LEFT |
		PSP_CTRL_CIRCLE | PSP_CTRL_SQUARE | PSP_CTRL_RTRIGGER | PSP_CTRL_LTRIGGER
	);
	
	/* QuickQuit値を初期化 */
	st_params->noqq = 0;
	
	/* ルートメニュー */
	st_params->proc = (MfFuncMenu)mf_root_menu;
	( st_params->proc )( MF_MM_INIT );
	
	dbgprint( "Starting menu-loop" );
	sceDisplayWaitVblankStart();
	
	while( gRunning ){
		/* 背景を描画 */
		if( st_params->frameBuffer.clear ){
			sceDmacMemcpy( gbGetCurrentDrawBuf(), st_params->frameBuffer.clear, st_params->frameBuffer.frameSize );
		} else if( st_params->frameBuffer.draw ){
			memset( gbGetCurrentDrawBuf(), 0, st_params->frameBuffer.frameSize );
			mf_draw_frame();
		} else{
			if( ( st_params->ctrl.pad->Buttons & 0x0000FFFF ) || st_params->screenUpdate ){
				gbWakeup();
				st_params->screenUpdate = false;
				memset( gbGetCurrentDrawBuf(), 0, st_params->frameBuffer.frameSize );
				mf_draw_frame();
			} else{
				gbSleep();
			}
		}
		
		padctrlPeekBuffer( st_params->ctrl.uid, st_params->ctrl.pad, 1 );
		padutilRemap( st_pref.remap, padutilSetPad( mfMenuGetCurrentButtons() ) | padutilSetHprm( mfMenuGetCurrentHprmKey() ), st_params->ctrl.pad, st_params->ctrl.hprmKey, true );
		
#ifdef DEBUG_ENABLED
		mf_debug();
#endif
		
		if( st_params->forcedQuit || ! st_params->proc || ( ! st_params->noqq && ( st_params->ctrl.pad->Buttons & ( PSP_CTRL_START | PSP_CTRL_HOME ) ) ) ) break;
		
		/* インジケータ表示 */
		mf_indicator();
		
		/* 情報バーをクリア */
		st_params->info.text[0] = '\0';
		st_params->info.options = 0;
		
		st_params->nextproc = st_params->proc;
		( st_params->proc )( st_params->menu.extra ? MF_MM_EXTRA : MF_MM_PROC );
		
		/* 情報バーを描画 */
		
		gbPrint(
			gbOffsetChar( 3 ),
			gbOffsetLine( 32 ) - ( st_params->info.options & MF_MENU_INFOTEXT_SET_MIDDLE_LINE ? 0 : ( GB_CHAR_HEIGHT >> 1 ) ),
			st_params->info.options & MF_MENU_INFOTEXT_ERROR ? MF_COLOR_TEXT_FC : MF_COLOR_TEXT_FG,
			MF_COLOR_TEXT_BG,
			st_params->info.text
		);
		
		if( ! st_params->nextproc ){
			dbgprint( "Menu back" );
			( st_params->proc )( MF_MM_TERM );
			mf_menu_stack( &(st_params->stack), MF_MENU_STACK_POP, &(st_params->proc) );
		} else if( st_params->nextproc != st_params->proc ){
			dbgprint( "Menu enter" );
			mf_menu_stack( &(st_params->stack), MF_MENU_STACK_PUSH, st_params->proc );
			st_params->proc = st_params->nextproc;
			( st_params->proc )( MF_MM_INIT );
		}
		
		/* 表示バッファと描画バッファを切り替える */
		sceDisplayWaitVblankStart();
		gbSwapBuffers();
		
		/* 指定時間待つ */
		if( st_params->waitMicroSecForUpdate ){
			sceKernelDelayThread( st_params->waitMicroSecForUpdate );
			st_params->waitMicroSecForUpdate = 0;
			st_params->screenUpdate = true;
			sceDisplayWaitVblankStart();
		}
	}
	
	gbPrint( gbOffsetChar( 30 ), gbOffsetLine( 17 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "Returning to the game..." );
	sceDisplayWaitVblankStart();
	gbSwapBuffers();
	sceKernelDelayThread( 300000 );
	
	/* メニュースタックに詰まれているメニュー全てにMF_MM_TERMを送る */
	while( st_params->proc ){
		( st_params->proc )( MF_MM_TERM );
		mf_menu_stack( &(st_params->stack), MF_MENU_STACK_POP, &(st_params->proc) );
	}
	
	mfMenuDrawDialog( mfDialogCurrentType(), false );
	mfDialogFinish();
	
	goto DESTROY;
		
	DESTROY:
		mf_menu_stack( &(st_params->stack), MF_MENU_STACK_DESTROY, NULL );
		
		padctrlDestroy( st_params->ctrl.uid );
		padutilDestroyButtonList( st_symbols );
		st_symbols = NULL;
		
		mf_free_buffers( &(st_params->frameBuffer) );
		
		/* サウンドを戻す */
		mf_menu_audio( true );
		
		/* メニュー起動前にAPIをフックしていた場合は、フックしなおす */
		if( hooked ) mfHook();
		
		/* 他のスレッドをリジューム */
		sceDisplayWaitVblankStart();
		mf_menu_control_thread_stat( MF_MENU_RESUME_THREADS, st_params->threads.list, st_params->threads.count );
		
		/* スレッドリスト用のメモリを解放 */
		dbgprint( "Free memory for menu" );
		memoryFree( st_params );
		
		dbgprintf( "Memory block remain count %d", memoryGetAllocCount() );
		
		/* LCDの点灯を待つ */
		sceDisplayWaitVblankStart();
		sceDisplayWaitVblankStart();
		sceDisplayEnable();
}

void mfMenuSetInfoText( unsigned int options, char *format, ... )
{
	va_list ap;
	char *strtop = st_params->info.text;
	
	if( ! st_params ) return;
	
	st_params->info.options = options;
	
	if( ( options & MF_MENU_INFOTEXT_COMMON_CTRL ) ){
		if( options & MF_MENU_INFOTEXT_MULTICOLUMN_CTRL ){
			strtop += snprintf( strtop, MF_MENU_INFOTEXT_LENGTH, "(\x80\x81\x82\x83)Move, " );
		} else if( options & MF_MENU_INFOTEXT_MOVABLEPAGE_CTRL ){
			strtop += snprintf( strtop, MF_MENU_INFOTEXT_LENGTH, "(\x80\x82)Move, (\x83\x81)PageMove, " );
		} else{
			strtop += snprintf( strtop, MF_MENU_INFOTEXT_LENGTH, "(\x80\x82)Move, " );
		}
		
		strtop += snprintf( strtop, MF_MENU_INFOTEXT_LENGTH, "(\x86)Back, (START/HOME)Exit" );
	}
	
	if( options & MF_MENU_INFOTEXT_SET_LOWER_LINE ){
		*strtop = '\n';
		strtop++;
	}
	
	va_start( ap, format );
	vsnprintf( strtop, MF_MENU_INFOTEXT_LENGTH - (unsigned int)( strtop - st_params->info.text ), format, ap );
	va_end( ap );
}

MfMenuTable *mfMenuCreateTables( unsigned short tables, ... )
{
	/*
		指定されたテーブルを全てメモリに一度に割り当てる。
		
		可変引数は、必ず二つで一組で、「テーブルの行/テーブルの列」を渡す。
		    mfMenuCreateTable(
		    	2,
		    	3, 1,
		    	5, 2
		    )；
		これは、二つのテーブルが存在し、その二つのテーブルはそれぞれ行/列が以下のようになるように割り当てられる。
		    テーブル1: 行3 列1
		    テーブル2: 行5 列2
		テーブル数が一致しない、引数を二つ一組で渡さない場合の動作は予測できない。
	*/
	
	va_list ap;
	MfMenuTable *table;
	SceSize allocsize;
	unsigned short i, j, k;
	struct {
		unsigned short rows;
		unsigned short cols;
	} tablecells[16];
	uintptr_t offset;
	
	if( tables >= 16 ) return NULL;
	
	allocsize = tables * sizeof( MfMenuTable );
	
	va_start( ap, tables );
	for( i = 0; i < tables; i++ ){
		tablecells[i].rows = va_arg( ap, int );
		tablecells[i].cols = va_arg( ap, int );
		
		dbgprintf( "Table size: %d, %d", tablecells[i].rows, tablecells[i].cols );
		
		allocsize += sizeof( MfMenuEntry * ) * tablecells[i].rows + sizeof( MfMenuEntry   ) * tablecells[i].cols;
		if( tablecells[i].cols > 1 ) allocsize += sizeof( unsigned int ) * tablecells[i].cols;
	}
	va_end( ap );
	
	dbgprintf( "Allocating memory for menu-tables: %d bytes", allocsize );
	table = (MfMenuTable *)memoryAlloc( allocsize );
	if( ! table ){
		dbgprint( "Failed to allocate memory" );
		return NULL;
	}
	
	offset = (uintptr_t)table + sizeof( MfMenuTable ) * tables;
	
	for( i = 0; i < tables; i++ ){
		table[i].label = NULL;
		table[i].x     = 0;
		table[i].y     = 0;
		table[i].rows  = tablecells[i].rows;
		table[i].cols  = tablecells[i].cols;
		if( table[i].cols > 1 ){
			unsigned int size = sizeof( unsigned int ) * table[i].cols;
			table[i].colsWidth = (unsigned int *)offset;
			memset( table[i].colsWidth, 0, size );
			offset += size;
		} else{
			table[i].colsWidth = NULL;
		}
		
		table[i].entry = (MfMenuEntry **)offset;
		offset += sizeof( MfMenuEntry * ) * table[i].rows;
		
		for( j = 0; j < table[i].rows; j++ ){
			table[i].entry[j] = (MfMenuEntry *)offset;
			for( k = 0; k < table[i].cols; k++ ){
				table[i].entry[j][k].active = false;
				table[i].entry[j][k].ctrl   = NULL;
				table[i].entry[j][k].label  = "";
				table[i].entry[j][k].var    = NULL;
				table[i].entry[j][k].arg    = NULL;
			}
			offset += sizeof( MfMenuEntry ) * table[i].cols;
		}
	}
	
	return table;
}

void mfMenuDestroyTables( MfMenuTable *table )
{
	memoryFree( table );
}

static void mf_indicator( void )
{
	static char lst_working_indicator[] = "|/-\\";
	static int  lst_working_indicator_index = 0;
	
	if( lst_working_indicator[lst_working_indicator_index] == '\0' ) lst_working_indicator_index = 0;
	
	gbPutChar( gbOffsetChar( 1 ), gbOffsetLine( 1 ), MF_COLOR_TITLE, GB_TRANSPARENT, lst_working_indicator[lst_working_indicator_index++] );
}

static bool mf_menu_stack( struct mf_menu_stack *stack, enum mf_menu_stack_ctrl ctrl, void *menu )
{
	switch( ctrl ){
		case MF_MENU_STACK_CREATE:
			stack->index = 0;
			stack->size  = MF_MENU_STACK_SIZE( MF_MENU_STACK_NUM );
			stack->memory = memoryAlloc( stack->size );
			dbgprintf( "Allocating memory for menu-stack: %p: %d bytes", stack->memory, stack->size );
			return stack->memory ? true : false;
		case MF_MENU_STACK_PUSH:
			if( stack->index >= MF_MENU_STACK_NUM ){
				void *old_stack = stack->memory;
				stack->size += MF_MENU_STACK_SIZE( MF_MENU_STACK_NUM );
				stack->memory = (MfProc *)memoryRealloc( stack->memory, stack->size );
				dbgprintf( "Re-allocating memory for menu-stack: %p: %d bytes", stack->memory, stack->size );
				if( ! stack->memory ){
					dbgprint( "Failed to re-allocate memory" );
					stack->memory = old_stack;
					return false;
				}
			}
			dbgprintf( "Menu address push to menu-stack: %p", menu );
			stack->memory[stack->index] = menu;
			stack->index++;
			return true;
		case MF_MENU_STACK_POP:
			if( stack->index ){
				stack->index--;
				dbgprintf( "Menu address pop from menu-stack: %p", stack->memory[stack->index] );
				*((void **)menu) = stack->memory[stack->index];
				return true;
			} else{
				*((void **)menu) = NULL;
				return false;
			}
		case MF_MENU_STACK_DESTROY:
			memoryFree( stack->memory );
			return true;
	}
	return false;
}

static bool mf_menu_get_target_threads( SceUID *thread_id_list, int *thread_id_count )
{
	int i, j;
	SceUID selfid;
	
	selfid = sceKernelGetThreadId();
	
	if( sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, thread_id_list, MF_MENU_MAX_NUMBER_OF_THREADS, thread_id_count ) < 0 ){
		dbgprint( "Failed to get a thread-id list" );
		return false;
	}
	
	for( i = 0; i < *thread_id_count; i++ ){
		for( j = 0; j < st_thread_id_count_first; j++ ){
			if( ( thread_id_list[i] == st_thread_id_list_first[j] ) || ( thread_id_list[i] == selfid ) ) thread_id_list[i] = 0;
		}
	}
	
	return true;
}

static void mf_menu_control_thread_stat( bool stat, SceUID *thread_id_list, int thread_id_count )
{
	int ( *func )( SceUID ) = NULL;
	int i;
	
	if( stat == MF_MENU_SUSPEND_THREADS ){
		dbgprint( "Get suspend request for all-Threads" );
		func = sceKernelSuspendThread;
	} else if( stat == MF_MENU_RESUME_THREADS ){
		dbgprint( "Get resume request for all-Threads" );
		func = sceKernelResumeThread;
	}
	
	if( ! func || ! thread_id_list || ! thread_id_count ) return;
	
	dbgprint( "Controling all-threads" );
	for( i = 0; i < thread_id_count; i++ ){
		if( thread_id_list[i] ) ( func )( thread_id_list[i] );
	}
}

static bool mf_alloc_buffers( struct mf_frame_buffers *fb )
{
	int vram_size = sceGeEdramGetSize();
	
	memset( fb, 0, sizeof( struct mf_frame_buffers ) );
	
	fb->vrambase  = (void *)( (unsigned int)sceGeEdramGetAddr() | GB_NOCACHE );
	fb->origaddr  = gbGetCurrentDisplayBuf();
	fb->frameSize = gbGetDataFrameSize();
	
	if( st_pref.useVolatileMem && scePowerVolatileMemTryLock( 0, &(fb->vram), &vram_size ) == 0 ){
		dbgprintf( "Memory allocated from 4MB extra RAM: %d bytes", vram_size );
		memcpy( fb->vram, fb->vrambase, vram_size );
		fb->display = fb->vrambase;
		fb->draw    = (void *)( (unsigned int)(fb->display) + fb->frameSize );
		fb->clear   = (void *)( (unsigned int)(fb->draw) + fb->frameSize );
		fb->volatileMem   = true;
	} else{
		unsigned int   free_mem  = sceKernelMaxFreeMemSize();
		unsigned short frame_num =  free_mem / fb->frameSize;
		
		if( free_mem >= vram_size ){
			fb->vram = memoryAlloc( vram_size );
			if( ! fb->vram ) return false;
			
			dbgprintf( "Memory allocated: %d bytes", vram_size );
			memcpy( fb->vram, fb->vrambase, vram_size );
			fb->display = fb->vrambase;
			fb->draw    = (void *)( (unsigned int)(fb->display) + fb->frameSize );
			fb->clear   = (void *)( (unsigned int)(fb->draw) + fb->frameSize );
		} else if( frame_num >= 2 ){
			fb->display = fb->origaddr;
			fb->draw    = memoryAlign( 16, fb->frameSize * 2 );
			if( ! fb->draw ) return false;
			dbgprintf( "Memory allocated for draw/clear buffers: %d bytes", fb->frameSize *2 );
			fb->clear = (void *)( (unsigned int)(fb->draw) + fb->frameSize );
		} else if( frame_num >= 1 ){
			fb->display = fb->origaddr;
			fb->draw    = memoryAlign( 16, fb->frameSize );
			if( ! fb->draw ) return false;
			dbgprintf( "Memory allocated for draw buffer: %d bytes", fb->frameSize );
		} else{
			dbgprint( "Not enough available memory: only use current display buffer" );
			fb->display = fb->origaddr;
		}
	}
	return true;
}

static void mf_free_buffers( struct mf_frame_buffers *fb )
{
	if( fb->vram ){
		dbgprint( "Restore vram buffer" );
		sceDisplayWaitVblankStart();
		sceDisplaySetFrameBuf( fb->origaddr, gbGetBufferWidth(), gbGetPixelFormat(), PSP_DISPLAY_SETBUF_IMMEDIATE );
		memcpy( fb->vrambase, fb->vram, sceGeEdramGetSize() );
	}
	
	dbgprint( "Free memory for frame buffers" );
	if( fb->volatileMem ){
		scePowerVolatileMemUnlock( 0 );
	} else if( fb->vram ){
		memoryFree( fb->vram );
	} else if( fb->draw ){
		memoryFree( fb->draw );
	}
	
}

static void mf_draw_frame( void )
{
	/* これらのセクション名は、内容が同じである場合、ポインタレベルで同じである */
	const char *reqid = mfGetIniRequestSection();
	const char *trgid = mfGetIniTargetSection();
	
	gbFillRect( 0, 0, SCR_WIDTH, gbOffsetLine( 3 ), MF_COLOR_TITLE_BAR );
	gbFillRect( 0, SCR_HEIGHT - gbOffsetLine( 3 ), SCR_WIDTH, SCR_HEIGHT, MF_COLOR_INFO_BAR );
	gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 1 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_TITLE, MF_VERSION );
	gbPrint( gbOffsetChar( 47 ), SCR_HEIGHT - gbOffsetLine( 1 ), ( MF_COLOR_TEXT_FG & 0x00ffffff ) | 0x33000000, MF_COLOR_TEXT_BG, MF_AUTHOR );
	
	gbPrintf( gbOffsetChar( 32 ), gbOffsetLine( 1 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "[ID: %s", reqid );
	if( reqid == trgid ){
		gbPrint( gbOffsetChar( 32 + 5 + strlen( reqid ) ), gbOffsetLine( 1 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "]" );
	} else{
		gbPrintf( gbOffsetChar( 32 + 5 + strlen( reqid ) ), gbOffsetLine( 1 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, " / Loaded: %s]", trgid );
	}
	
	if( mfHookIncomplete() ){
		gbPrint( gbOffsetChar( 44 ), gbOffsetLine( 2 ) + ( gbOffsetLine( 1 ) >> 1 ), MF_COLOR_EX4, MF_COLOR_TEXT_BG, "MacroFire is not working perfectly." );
	}
	
#ifdef MF_WITH_EXCEPTION_HANDLER
		gbPrint( 0, 0, MF_COLOR_EX4, MF_COLOR_TEXT_BG, "<for DEBUG>" );
#endif
}

/*=========================================================

	mfmenu.c

	MacroFire メニュー

=========================================================*/
#include "mfmenu.h"
#include <pspge.h>
#include <pspimpose_driver.h>
#include <pspsdk.h>
#include <pspsuspend.h>
#include <psputility.h>
#include <stdarg.h>

/*=========================================================
	ローカル定数
=========================================================*/
#define MF_MENU_MAX_NUMBER_OF_THREADS 64

#define MF_MENU_STACK_SIZE( x ) ( sizeof( struct MfMenuStackData ) * ( x ) )
#define MF_MENU_STACK_NUM       8

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
struct MfMenuFrameBuffer {
	void         *OrigAddr;
	unsigned int Size;
	int  Width;
	int  PixelFormat;
	
	void *VramBase;
	void *Vram;
	void *Disp;
	void *Draw;
	void *Clear;
};

struct MfMenuThreads {
	SceUID List[MF_MENU_MAX_NUMBER_OF_THREADS];
	int    Count;
};

struct MfMenuPosition {
	unsigned short CurTable;
	unsigned short CurRow;
	unsigned short CurCol;
};

struct MfMenuMessage {
	MfMenuMessage   Main;
	MfMessage       Sub;
};

struct MfMenuExtra {
	MfMenuExtraProc Proc;
	void            *Argp;
};

struct MfMenuStackData {
	MfProc *Proc;
	struct MfMenuPosition Pos;
};

struct MfMenuStack {
	struct MfMenuStackData *Data;
	unsigned int Index;
	unsigned int Size;
};

struct MfMenuInput {
	SceCtrlData *Pad;
	MfHprmKey   *HprmKey;
	PadctrlUID  Uid;
	
	struct {
		unsigned int Button;
		char *Symbol;
	} Accept, Cancel;
};

struct MfMenuPref {
	bool LowMemoryMode;
	PadutilRemap *Remap;
};

struct MfMenuInfoBar {
	char Text[MF_MENU_INFOTEXT_LENGTH];
	unsigned int Options;
};

struct MfMenuParams {
	struct MfMenuFrameBuffer FrameBuffer;
	struct MfMenuThreads     Thread;
	struct MfMenuPosition    Menu;
	struct MfMenuInput       Ctrl;
	struct MfMenuStack       Stack;
	struct MfMenuInfoBar     Info;
	struct MfMenuMessage     CurrentMessage;
	struct MfMenuMessage     NextMessage;
	struct MfMenuExtra       Extra;
	PadutilButtonName        *ButtonSymbolTable;
	MfDialogType LastDialogType;
	unsigned int  Flags;
	unsigned int  WaitMicroSecForUpdate;
	MfFuncMenu Proc, NextProc;
} ;

enum MfMenuStackAction {
	MF_MENU_STACK_CREATE,
	MF_MENU_STACK_PUSH,
	MF_MENU_STACK_POP,
	MF_MENU_STACK_DESTROY
};

enum MfMenuMoveDirection {
	MF_MENU_MOVE_UP,
	MF_MENU_MOVE_RIGHT,
	MF_MENU_MOVE_DOWN,
	MF_MENU_MOVE_LEFT
};
	
struct MfMenuMainPref{
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
static bool mf_menu_init_params( struct MfMenuParams *params, SceCtrlData *pad, MfHprmKey *hk );
static bool mf_menu_init_graphics( void );
static bool mf_menu_get_target_threads( SceUID *thread_id_list, int *thread_id_count );
static void mf_menu_change_threads_stat( bool stat, SceUID *thread_id_list, int thread_id_count );
static bool mf_alloc_buffers( struct MfMenuFrameBuffer *fb, bool graphics_mem );
static void mf_free_buffers( struct MfMenuFrameBuffer *fb );
static void mf_draw_frame( bool lowmem );
static void mf_menu_send_message_for_items( MfMenuTable *menu, unsigned short menucnt, MfMessage message );
static bool mf_menu_move_table( enum MfMenuMoveDirection dir, MfMenuTable *menu, unsigned short menucnt, struct MfMenuPosition *pos );
static bool mf_menu_nearest_table( unsigned int pri, unsigned int sec, unsigned int *save_pri, unsigned int *save_sec );
static bool mf_menu_farest_table( unsigned int pri, unsigned int sec, unsigned int *save_pri, unsigned int *save_sec );
static bool mf_menu_select_table_vertical( MfMenuTable *menu, unsigned short targ_tid, struct MfMenuPosition *pos, bool reverse );
static bool mf_menu_select_table_horizontal( MfMenuTable *menu, unsigned short targ_tid, struct MfMenuPosition *pos, bool reverse );
static bool mf_menu_stack( struct MfMenuStack *stack, enum MfMenuStackAction ctrl, void *menu, struct MfMenuPosition *pos );
static unsigned int mf_label_width( char *str );
static void mf_root_menu( MfMenuMessage message );
bool mf_control_engine( MfMessage message, const char *label, void *var, void *unused, void *ex );
static void mf_menu_mute( bool mute );
static void mf_menu_indicator( void );

/*=========================================================
	ローカル変数
=========================================================*/
static struct MfMenuThreads st_systhread;
static struct MfMenuPref    st_pref;
static struct MfMenuParams  *st_params;

/*=========================================================
	関数
=========================================================*/
#ifdef DEBUG_ENABLED
static void mf_debug( void )
{
	static int fps = 0;
	static int st_fps = 0;
	static uint64_t st_fps_tick_now = 0;
	static uint64_t st_fps_tick_last = 0;
	
	st_fps++;
	sceRtcGetCurrentTick( &st_fps_tick_now );
	if( (st_fps_tick_now - st_fps_tick_last) >= 1000000 ){
		st_fps_tick_last = st_fps_tick_now;
		fps = st_fps;
		st_fps = 0;
	}
		
	pbPrintf( pbOffsetChar( 5 ), pbOffsetLine( 2 ), 0xff00ffff, MF_COLOR_TEXT_BG,
		"FPS:%d T:%d M:%d (KH:%d U:%d KL:%d V:%d)",
		fps,
		sceKernelTotalFreeMemSize(),
		sceKernelMaxFreeMemSize(),
		sceKernelPartitionTotalFreeMemSize( 1 ),
		sceKernelPartitionTotalFreeMemSize( 2 ),
		sceKernelPartitionTotalFreeMemSize( 4 ),
		sceKernelPartitionTotalFreeMemSize( 5 )
	);
	pbPrintf( pbOffsetChar( 1 ), pbOffsetLine( 0 ), 0xff0000ff, MF_COLOR_TEXT_BG, "Development build - %s", __DATE__ );
}
#endif

/*-----------------------------------------------
	メニューの初期化。
	
	メニューを呼び出した際に他スレッドを停止させる為に使用するスレッドリストを取得。
	このスレッドリストに入っているスレッドIDは、メニュー起動時に停止しない。
-----------------------------------------------*/
bool mfMenuInit( void )
{
	if( sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, st_systhread.List, MF_MENU_MAX_NUMBER_OF_THREADS, &(st_systhread.Count) ) < 0 ){
		dbgprint( "Failed to get a first thread-id list" );
		return false;
	}
	sceCtrlSetSamplingMode( PSP_CTRL_MODE_ANALOG );
	
	/* 設定初期化 */
	st_pref.LowMemoryMode = false;
	st_pref.Remap         = NULL;
	
	return true;
}

bool mfMenuAddSysThreadId( SceUID thid )
{
	if( st_systhread.Count >= MF_MENU_MAX_NUMBER_OF_THREADS ) return false;
	st_systhread.List[st_systhread.Count++] = thid;
	return true;
}

/*-----------------------------------------------
	メニューの終了
	
	AlternativeButtons設定が行われていれば破棄する。
-----------------------------------------------*/
void mfMenuDestroy( void )
{
	if( st_pref.Remap ) padutilDestroyRemapArray( st_pref.Remap );
}

/*-----------------------------------------------
	設定ファイルのロード
-----------------------------------------------*/
void mfMenuIniLoad( IniUID ini, char *buf, size_t len )
{
	unsigned short i, j;
	const char *section = mfGetIniSection();
	PadutilButtonName button_names[] = { MF_MENU_ALT_BUTTONS };
	
	for( i = 0, j = 0; i < MF_MENU_ALT_BUTTONS_COUNT; i++ ){
		if( inimgrGetString( ini, "AlternativeButtons", button_names[i].name, buf, len ) > 0 ){
			if( ! st_pref.Remap ) st_pref.Remap = padutilCreateRemapArray( MF_MENU_ALT_BUTTONS_COUNT );
			st_pref.Remap[j].realButtons = mfConvertButtonN2C( buf );
			st_pref.Remap[j].remapButtons  = button_names[i].button;
		}
	}
	inimgrGetBool( ini, section, "ForcedLowMemoryMode", &(st_pref.LowMemoryMode) );
}

void mfMenuIniSave( IniUID ini, char *buf, size_t len )
{
	unsigned short i;
	PadutilButtonName button_names[] = { MF_MENU_ALT_BUTTONS };
	
	for( i = 0; i < MF_MENU_ALT_BUTTONS_COUNT; i++ ){
		strlwr( button_names[i].name );
		button_names[i].name[0] = toupper( button_names[i].name[0] );
		
		inimgrSetString( ini, "AlternativeButtons", button_names[i].name, "" );
	}
	
	inimgrSetBool( ini, MF_INI_SECTION_DEFAULT, "ForcedLowMemoryMode", false );
}

void mfMenuWait( unsigned int microsec )
{
	st_params->WaitMicroSecForUpdate = microsec;
}

void mfMenuProc( MfMenuProc proc )
{
	st_params->NextProc = proc;
}

bool mfMenuSetExtra( MfMenuExtraProc extra, void *argp )
{
	if( st_params->Extra.Proc ) return false;
	
	st_params->Extra.Proc = extra;
	st_params->Extra.Argp = argp;
	( st_params->Extra.Proc )( MF_MM_INIT, st_params->Extra.Argp );
	
	return true;
}

void mfMenuExitExtra( void )
{
	( st_params->Extra.Proc )( MF_MM_TERM, st_params->Extra.Argp );
	st_params->Extra.Proc = NULL;
	st_params->Extra.Argp = NULL;
	
	mfMenuResetKeyRepeat();
}

void mfMenuSendSignal( MfSignalMessage sig )
{
	st_params->NextMessage.Sub = sig;
}

void mfMenuEnable( unsigned int flags )
{
	st_params->Flags |= flags;
}

void mfMenuDisable( unsigned int flags )
{
	st_params->Flags &= ~flags;
}

unsigned int mfMenuGetFlags( void )
{
	return st_params->Flags;
}

char *mfMenuButtonsSymbolList( PadutilButtons buttons, char *str, size_t len )
{
	padutilGetButtonNamesByCode( st_params->ButtonSymbolTable, buttons, " ", 0, str, len );
	return str;
}

bool mfMenuDrawDialog( MfDialogType dialog, bool update )
{
	switch( dialog ){
		case MF_DIALOG_MESSAGE:
			if( update   ) update = mfDialogMessageDraw();
			if( ! update ) mfDialogMessageResult();
			return true;
		case MF_DIALOG_SOSK:
			if( update   ) update = mfDialogSoskDraw();
			if( ! update ) mfDialogSoskResult();
			return true;
		case MF_DIALOG_NUMEDIT:
			if( update   ) update = mfDialogNumeditDraw();
			if( ! update ) mfDialogNumeditResult();
			return true;
		case MF_DIALOG_GETFILENAME:
			if( update   ) update = mfDialogGetfilenameDraw();
			if( ! update ) mfDialogGetfilenameResult();
			return true;
		case MF_DIALOG_DETECTBUTTONS:
			if( update   ) update = mfDialogDetectbuttonsDraw();
			if( ! update ) mfDialogDetectbuttonsResult();
			return true;
		default:
			return false;
	}
}

void mfMenuInitTables( MfMenuTable menu[], unsigned short menucnt )
{
	st_params->Menu.CurTable = 0;
	st_params->Menu.CurRow   = 0;
	st_params->Menu.CurCol   = 0;
	
	mf_menu_send_message_for_items( menu, menucnt, MF_CM_INIT );
}

void mfMenuTermTables( MfMenuTable menu[], unsigned short menucnt )
{
	mf_menu_send_message_for_items( menu, menucnt, MF_CM_TERM );
}

bool mfMenuDrawTables( MfMenuTable menu[], unsigned short cnt, unsigned int opt )
{
	/* 選択可能な項目(コントロールが設定されていてActivateされている)が一つもないと無限ループ */
	
	unsigned short i, row, col;
	unsigned int *col_width, width, col_offset, row_offset;
	char labelbuf[MF_CTRL_BUFFER_LENGTH];
	struct MfMenuPosition dupe_pos;
	bool refind = true;
	
	/* ラベルを描画 */
	for( i = 0; i < cnt; i++ ){
		if( menu[i].label ){
			col_offset = pbOffsetChar( 2 );
			row_offset = pbOffsetLine( 1 ) + 2;
			pbPrint( menu[i].x, menu[i].y,  MF_COLOR_TEXT_LABEL, MF_COLOR_TEXT_BG, menu[i].label );
			pbLine( menu[i].x + pbMeasureString( menu[i].label ) + ( pbOffsetChar( 1 ) >> 1 ), menu[i].y + ( pbOffsetLine( 1 ) >>1 ), SCR_WIDTH - pbOffsetChar( 6 ), menu[i].y + ( pbOffsetLine( 1 ) >>1 ), MF_COLOR_TEXT_LABEL );
		} else{
			col_offset = 0;
			row_offset = 0;
		}
		for( col = 0; col < menu[i].cols; col++ ){
			col_width = ( menu[i].colsWidth && ( menu[i].colsWidth[col] == 0 ) ) ? &(menu[i].colsWidth[col]) : NULL;
			if( menu[i].colsWidth && col ) col_offset += menu[i].colsWidth[col - 1] + 1;
			for( row = 0; row < menu[i].rows; row++ ){
				labelbuf[0] = '\0';
				if( menu[i].entry[row][col].ctrl ){
					( menu[i].entry[row][col].ctrl )(
						MF_CM_LABEL | st_params->CurrentMessage.Sub,
						menu[i].entry[row][col].label,
						menu[i].entry[row][col].var,
						menu[i].entry[row][col].arg,
						labelbuf
					);
				}
				if( labelbuf[0] == '\0' ) strutilCopy( labelbuf, menu[i].entry[row][col].label, sizeof( labelbuf ) );
				
				if( labelbuf[0] != '\0' ){
					pbPrint(
						menu[i].x + col_offset,
						menu[i].y + row_offset + pbOffsetLine( row ),
						(
							i == st_params->Menu.CurTable && row == st_params->Menu.CurRow && col == st_params->Menu.CurCol ? MF_COLOR_TEXT_FC :
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
	
	
	if( ! ( opt & MF_MENU_DISPLAY_ONLY ) ){
		MfCtrlMessage ctrlmsg;
		
		if( st_params->Extra.Proc ){
			/* 拡張関数が設定されていれば実行 */
			( st_params->Extra.Proc )( MF_MM_PROC | st_params->CurrentMessage.Sub, st_params->Extra.Argp );
			ctrlmsg = ( st_params->Extra.Proc ? MF_CM_WAIT : MF_CM_CONTINUE );
		} else if( mfMenuMaskDialogMessage( st_params->CurrentMessage.Sub ) ){
			ctrlmsg = ( ! ( st_params->CurrentMessage.Sub & MF_DM_FINISH ) ? MF_CM_WAIT : MF_CM_CONTINUE );
		} else{
			ctrlmsg = MF_CM_PROC;
		}
		
		if( menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][st_params->Menu.CurCol].active ){
			if( menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][st_params->Menu.CurCol].ctrl ){
				if( !
					( menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][st_params->Menu.CurCol].ctrl )(
						ctrlmsg | st_params->CurrentMessage.Sub,
						menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][st_params->Menu.CurCol].label,
						menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][st_params->Menu.CurCol].var,
						menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][st_params->Menu.CurCol].arg,
						NULL
					)
				) return false;
			}
			if( st_params->CurrentMessage.Sub & ( MF_DM_START | MF_DM_UPDATE ) ) return true;
		}
	} else{
		return true;
	}
	
	/* 拡張関数が設定されていればここまで */
	if( st_params->Extra.Proc ) return true;
	
	/* BACKの検出 */
	if( mfMenuIsPressed( mfMenuCancelButton() ) ) return false;
	
	/* 情報バーのデータを取得 */
	{
		unsigned int opt = menu[st_params->Menu.CurTable].cols == 1 ? 0 : MF_MENU_INFOTEXT_MULTICOLUMN_CTRL;
		
		if( menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][st_params->Menu.CurCol].active ){
			( menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][st_params->Menu.CurCol].ctrl )(
				MF_CM_INFO | st_params->CurrentMessage.Sub,
				menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][st_params->Menu.CurCol].label,
				menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][st_params->Menu.CurCol].var,
				menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][st_params->Menu.CurCol].arg,
				&opt
			);
		}
		if( st_params->Info.Text[0] == '\0' ) mfMenuSetInfoText( MF_MENU_INFOTEXT_COMMON_CTRL, "" );
	}
	
	/* 現在の位置をバックアップ */
	dupe_pos = st_params->Menu;
	
	/* 操作 */
	while( refind ){
		refind = false;
		
		if( mfMenuIsPressed( PSP_CTRL_UP ) ){
			if( st_params->Menu.CurRow ){
				st_params->Menu.CurRow--;
			} else if( ! mf_menu_move_table( MF_MENU_MOVE_UP, menu, cnt, &(st_params->Menu) ) ){
				refind = true;
				break;
			}
		} else if( mfMenuIsPressed( PSP_CTRL_DOWN ) ){
			if( st_params->Menu.CurRow < menu[st_params->Menu.CurTable].rows - 1 ){
				st_params->Menu.CurRow++;
			} else if( ! mf_menu_move_table( MF_MENU_MOVE_DOWN, menu, cnt, &(st_params->Menu) ) ){
				refind = true;
				break;
			}
		} else if( mfMenuIsPressed( PSP_CTRL_LEFT ) ){
			if( st_params->Menu.CurCol ){
				st_params->Menu.CurCol--;
			} else if( ! mf_menu_move_table( MF_MENU_MOVE_LEFT, menu, cnt, &(st_params->Menu) ) ){
				refind = true;
				break;
			}
		} else if( mfMenuIsPressed( PSP_CTRL_RIGHT ) ){
			if( st_params->Menu.CurCol < menu[st_params->Menu.CurTable].cols - 1 ){
				st_params->Menu.CurCol++;
			} else if( ! mf_menu_move_table( MF_MENU_MOVE_RIGHT, menu, cnt, &(st_params->Menu) ) ){
				refind = true;
				break;
			}
		}
		
		if( ! menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][st_params->Menu.CurCol].active ){
			short pos_pos, pos_neg;
			if( mfMenuIsAnyPressed( PSP_CTRL_UP | PSP_CTRL_DOWN ) ){
				refind = true;
				pos_pos = st_params->Menu.CurCol + 1;
				pos_neg = st_params->Menu.CurCol - 1;
				while( refind && ( pos_pos >= 0 || pos_neg >= 0 ) ){
					if( pos_pos >= menu[st_params->Menu.CurTable].cols ) pos_pos = -1;
					
					if( pos_neg >= 0 ){
						if( menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][pos_neg].active ){
							st_params->Menu.CurCol = pos_neg;
							refind = false;
						}
						pos_neg--;
					}else if( pos_pos >= 0 ){
						if( menu[st_params->Menu.CurTable].entry[st_params->Menu.CurRow][pos_pos].active ){
							st_params->Menu.CurCol = pos_pos;
							refind = false;
						}
						pos_pos++;
					}
				}
			} else if( mfMenuIsAnyPressed( PSP_CTRL_RIGHT | PSP_CTRL_LEFT ) ){
				refind = true;
				pos_pos = st_params->Menu.CurRow + 1;
				pos_neg = st_params->Menu.CurRow - 1;
				while( refind && ( pos_pos >= 0 || pos_neg >= 0 ) ){
					if( pos_pos >= menu[st_params->Menu.CurTable].rows ) pos_pos = -1;
					
					if( pos_neg >= 0 ){
						if( menu[st_params->Menu.CurTable].entry[pos_neg][st_params->Menu.CurCol].active ){
							st_params->Menu.CurRow = pos_neg;
							refind = false;
						}
						pos_neg--;
					}else if( pos_pos >= 0 ){
						if( menu[st_params->Menu.CurTable].entry[pos_pos][st_params->Menu.CurCol].active ){
							st_params->Menu.CurRow = pos_pos;
							refind = false;
						}
						pos_pos++;
					}
				}
			}
		}
	}
	
	if( refind ) st_params->Menu = dupe_pos;
	
	return true;
}

unsigned int mfMenuAcceptButton( void )
{
	return st_params->Ctrl.Accept.Button;
}

unsigned int mfMenuCancelButton( void )
{
	return st_params->Ctrl.Cancel.Button;
}

char *mfMenuAcceptSymbol( void )
{
	return st_params->Ctrl.Accept.Symbol;
}

char *mfMenuCancelSymbol( void )
{
	return st_params->Ctrl.Cancel.Symbol;
}

inline SceCtrlData *mfMenuGetCurrentPadData( void )
{
	return st_params->Ctrl.Pad;
}

inline unsigned int mfMenuGetCurrentButtons( void )
{
	return st_params->Ctrl.Pad->Buttons;
}

inline unsigned char mfMenuGetCurrentAnalogStickX( void )
{
	return st_params->Ctrl.Pad->Lx;
}

inline unsigned char mfMenuGetCurrentAnalogStickY( void )
{
	return st_params->Ctrl.Pad->Ly;
}

inline MfHprmKey mfMenuGetCurrentHprmKey( void )
{
	return *(st_params->Ctrl.HprmKey);
}

inline void mfMenuResetKeyRepeat( void )
{
	padctrlResetRepeat( st_params->Ctrl.Uid );
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
	tableid--;
	row--;
	col--;
	menu[tableid].entry[row][col].active = false;
}

void mfMenuActiveTable( MfMenuTable *menu, unsigned short tableid )
{
	unsigned short row, col;
	
	for( row = menu[tableid - 1].rows; row; row-- ){
		for( col = menu[tableid - 1].cols; col; col-- ){
			mfMenuActiveTableEntry( menu, tableid, row , col );
		}
	}
}

void mfMenuInactiveTable( MfMenuTable *menu, unsigned short tableid )
{
	unsigned short row, col;
	
	for( row = menu[tableid - 1].rows; row; row-- ){
		for( col = menu[tableid - 1].cols; col; col-- ){
			mfMenuInactiveTableEntry( menu, tableid, row , col );
		}
	}
}

static bool mf_menu_move_table( enum MfMenuMoveDirection dir, MfMenuTable *menu, unsigned short menucnt, struct MfMenuPosition *pos )
{
	unsigned short i;
	unsigned short target_table = pos->CurTable;
	unsigned int   save_diff_x  = ~0;
	unsigned int   save_diff_y  = ~0;
	
	/* 入力方向にもっとも近いテーブルを探す */
	dbgprint( "Out of menu-range. find a next table..." );
	switch( dir ){
		case MF_MENU_MOVE_UP:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->CurTable].y <= menu[i].y ) continue;
				if( mf_menu_nearest_table( abs( menu[pos->CurTable].y - menu[i].y ), abs( menu[pos->CurTable].x - menu[i].x ), &save_diff_y, &save_diff_x ) ){
					target_table = i;
				}
			}
			if( target_table != pos->CurTable ){
				return mf_menu_select_table_vertical( menu, target_table, pos, false );
			} else{
				break;
			}
		case MF_MENU_MOVE_RIGHT:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->CurTable].x >= menu[i].x ) continue;
				if( mf_menu_nearest_table( abs( menu[pos->CurTable].x - menu[i].x ), abs( menu[pos->CurTable].y - menu[i].y ), &save_diff_x, &save_diff_y ) ){
					target_table = i;
				}
			}
			if( target_table != pos->CurTable ){
				return mf_menu_select_table_horizontal( menu, target_table, pos, false );
			} else{
				break;
			}
		case MF_MENU_MOVE_DOWN:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->CurTable].y >= menu[i].y ) continue;
				if( mf_menu_nearest_table( abs( menu[pos->CurTable].y - menu[i].y ), abs( menu[pos->CurTable].x - menu[i].x ), &save_diff_y, &save_diff_x ) ){
					target_table = i;
				}
			}
			if( target_table != pos->CurTable ){
				return mf_menu_select_table_vertical( menu, target_table, pos, false );
			} else{
				break;
			}
		case MF_MENU_MOVE_LEFT:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->CurTable].x <= menu[i].x ) continue;
				if( mf_menu_nearest_table( abs( menu[pos->CurTable].x - menu[i].x ), abs( menu[pos->CurTable].y - menu[i].y ), &save_diff_x, &save_diff_y ) ){
					target_table = i;
				}
			}
			if( target_table != pos->CurTable ){
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
				if( menu[pos->CurTable].y >= menu[i].y ) continue;
				if( mf_menu_farest_table( abs( menu[pos->CurTable].y - menu[i].y ), abs( menu[pos->CurTable].x - menu[i].x ), &save_diff_y, &save_diff_x ) ){
					target_table = i;
				}
			}
			return mf_menu_select_table_vertical( menu, target_table, pos, true );
		case MF_MENU_MOVE_RIGHT:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->CurTable].x <= menu[i].x ) continue;
				if( mf_menu_farest_table( abs( menu[pos->CurTable].x - menu[i].x ), abs( menu[pos->CurTable].y - menu[i].y ), &save_diff_x, &save_diff_y ) ){
					target_table = i;
				}
			}
			return mf_menu_select_table_horizontal( menu, target_table, pos, true );
		case MF_MENU_MOVE_DOWN:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->CurTable].y <= menu[i].y ) continue;
				if( mf_menu_farest_table( abs( menu[pos->CurTable].y - menu[i].y ), abs( menu[pos->CurTable].x - menu[i].x ), &save_diff_y, &save_diff_x ) ){
					target_table = i;
				}
			}
			return mf_menu_select_table_vertical( menu, target_table, pos, true );
		case MF_MENU_MOVE_LEFT:
			for( i = 0; i < menucnt; i++ ){
				if( menu[pos->CurTable].x >= menu[i].x ) continue;
				if( mf_menu_farest_table( abs( menu[pos->CurTable].x - menu[i].x ), abs( menu[pos->CurTable].y - menu[i].y ), &save_diff_x, &save_diff_y ) ){
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

static bool mf_menu_select_table_horizontal( MfMenuTable *menu, unsigned short targ_tid, struct MfMenuPosition *pos, bool reverse )
{
	if( pos->CurTable == targ_tid ){
		dbgprintf( "Selected same table id: Col %d, %d", pos->CurCol, menu[targ_tid].cols );
		pos->CurCol = pos->CurCol ? 0 : menu[targ_tid].cols - 1;
	} else{
		unsigned short i, targ_row = 0, targ_col = 0;
		unsigned int current_y = menu[pos->CurTable].y + pbOffsetLine( pos->CurRow );
		unsigned int diff_y, last_diff_y = ~0;
		
		for( i = 0; i < menu[targ_tid].rows; i++ ){
			diff_y = abs( current_y - ( menu[targ_tid].y + pbOffsetLine( i ) ) );
			if( last_diff_y == ~0 || last_diff_y > diff_y ){
				last_diff_y = diff_y;
				targ_row = i;
			}
		}
		
		if( menu[pos->CurTable].x > menu[targ_tid].x ){
			targ_col = reverse ? 0 : menu[targ_tid].cols - 1;
		} else{
			targ_col = reverse ? menu[targ_tid].cols - 1 : 0;
		}
		
		pos->CurTable = targ_tid;;
		pos->CurRow   = targ_row;
		pos->CurCol   = targ_col;
	}
	
	return true;
}

static bool mf_menu_select_table_vertical( MfMenuTable *menu, unsigned short targ_tid, struct MfMenuPosition *pos, bool reverse )
{
	if( pos->CurTable == targ_tid ){
		dbgprintf( "Selected same table id: Row %d, %d", pos->CurRow, menu[targ_tid].rows );
		pos->CurRow = pos->CurRow ? 0 : menu[targ_tid].rows - 1;
	} else{
		unsigned short i, targ_row = 0, targ_col = 0;
		unsigned int current_x = menu[pos->CurTable].x;
		unsigned int comp_x, diff_x, last_diff_x = ~0;

		if( menu[pos->CurTable].colsWidth ){
			for( i = 0; i < pos->CurCol; i++ ){
				current_x += menu[pos->CurTable].colsWidth[i];
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
		
		if( menu[pos->CurTable].y > menu[targ_tid].y ){
			targ_row = reverse ? 0 : menu[targ_tid].rows - 1;
		} else{
			targ_row = reverse ? menu[targ_tid].rows - 1 : 0;
		}
		
		pos->CurTable = targ_tid;
		pos->CurRow   = targ_row;
		pos->CurCol   = targ_col;
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
	
	return pbOffsetChar( max_width ? max_width : strlen( str ) );
}

static void mf_menu_send_message_for_items( MfMenuTable *menu, unsigned short menucnt, MfMessage message )
{
	unsigned short i, row, col;
	
	dbgprintf( "Sending message %X to menu items...", message );
	for( i = 0; i < menucnt; i++ ){
		for( row = 0; row < menu[i].rows; row++ ){
			for( col = 0; col < menu[i].cols; col++ ){
				dbgprintf( "Init \"%s\" (%d,%d,%d)", menu[i].entry[row][col].label, i, row, col );
				if( menu[i].entry[row][col].ctrl ) ( menu[i].entry[row][col].ctrl )(
					message,
					menu[i].entry[row][col].label,
					menu[i].entry[row][col].var,
					menu[i].entry[row][col].arg,
					NULL
				);
			}
		}
	}
	dbgprint( "Completed" );
}

static void mf_root_menu( MfMenuMessage message )
{
	static MfMenuTable *menu;
	static HeapUID pheap;
	static struct MfMenuMainPref *main_pref;
	
	switch( message ){
		case MF_MM_INIT:
			dbgprint( "Initializing root-menu" );
			menu = mfMenuCreateTables(
				3,
				1, 1,
				4, 1,
				gMftabEntryCount, 1
			);
			pheap = mfMemoryTempHeapCreate( 2, sizeof( struct MfMenuMainPref ) + sizeof( MfCtrlDefGetButtonsPref ) );
			if( ! menu || ! pheap ){
				if( menu  ) mfMenuDestroyTables( menu );
				if( pheap ) mfMemoryHeapDestroy( pheap );
				return;
			}
			
			{
				unsigned short row;
				void *menuproc;
				MfCtrlDefGetButtonsPref *hotkey_pref = mfMemoryHeapCalloc( pheap, sizeof( MfCtrlDefGetButtonsPref ) );;
				main_pref = mfMemoryHeapCalloc( pheap, sizeof( struct MfMenuMainPref ) );
				main_pref->engine             = mfIsEnabled();
				main_pref->menu               = mfGetMenuButtons();
				main_pref->toggle             = mfGetToggleButtons();
				main_pref->statusNotification = mfNotificationThreadId() ? true : false;
				
				dbgprintf( "NTHID: %X", mfNotificationThreadId() );
				
				hotkey_pref->availButtons = MF_HOTKEY_BUTTONS;
				
				mfMenuSetTablePosition( menu, 1, pbOffsetChar( 5 ), pbOffsetLine( 4 ) + 2 );
				mfMenuSetTableEntry( menu, 1, 1, 1, MF_STR_HOME_MACROFIRE_ENGINE, mf_control_engine, &(main_pref->engine), NULL );
				
				mfMenuSetTablePosition( menu, 2, pbOffsetChar( 5 ), pbOffsetLine( 6 ) + 2 );
				mfMenuSetTableLabel( menu, 2, MF_STR_HOME_MACROFIRE_PREFERENCE );
				mfMenuSetTableEntry( menu, 2, 1, 1, MF_STR_HOME_SET_MENU_BUTTON, mfCtrlDefGetButtons, &(main_pref->menu), hotkey_pref );
				mfMenuSetTableEntry( menu, 2, 2, 1, MF_STR_HOME_SET_TOGGLE_BUTTON, mfCtrlDefGetButtons, &(main_pref->toggle), hotkey_pref );
				mfMenuSetTableEntry( menu, 2, 3, 1, MF_STR_HOME_STAT_NOTIFICATION, mfCtrlDefBool, &(main_pref->statusNotification), NULL );
				mfMenuSetTableEntry( menu, 2, 4, 1, MF_STR_HOME_ANALOG_STICK_ADJUSTMENT, mfCtrlDefCallback, mfAnalogStickMenu, NULL );
				
				mfMenuSetTablePosition( menu, 3, pbOffsetChar( 5 ), pbOffsetLine( 12 ) + 2 );
				mfMenuSetTableLabel( menu, 3, MF_STR_HOME_FUNCTION );
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
			dbgprint( "Terminating root-menu" );
			mfSetMenuButtons( main_pref->menu );
			mfSetToggleButtons( main_pref->toggle );
			
			if( mfNotificationThreadId() && ! main_pref->statusNotification ){
				dbgprint( "Notification thread is shutting down..." );
				mfNotificationShutdownStart();
			} else if( ! mfNotificationThreadId() && main_pref->statusNotification ){
				dbgprint( "Notification thread is starting..." );
				mfNotificationStart();
			}
			
			mfMenuDestroyTables( menu );
			mfMemoryHeapDestroy( pheap );
			return;
		default:
			if( ! mfMenuDrawTables( menu, 3, MF_MENU_NO_OPTIONS ) ) mfMenuProc( NULL );
	}
}

/*-----------------------------------------------
	メニュー メインループ
-----------------------------------------------*/
static bool mf_menu_init_params( struct MfMenuParams *params, SceCtrlData *pad, MfHprmKey *hk )
{
	int ox_swap;
	
	/* メニュースタックを確保 */
	if( ! mf_menu_stack( &(st_params->Stack), MF_MENU_STACK_CREATE, NULL, NULL ) ) return false;
	
	/* ボタンシンボルリスト生成 */
	st_params->ButtonSymbolTable = padutilCreateButtonSymbols();
	if( ! st_params->ButtonSymbolTable ) return false;
	
	/* ダイアログ初期化 */
	mfDialogInit( st_pref.Remap );
	st_params->LastDialogType = mfDialogCurrentType();
	
	/* コントロール */
	st_params->Ctrl.Pad     = pad;
	st_params->Ctrl.HprmKey = hk;
	st_params->Ctrl.Uid     = padctrlNew();
	if( st_params->Ctrl.Uid < 0 ){
		dbgprintf( "Failed to get a PadctrlUID: %x", st_params->Ctrl.Uid );
		st_params->Ctrl.Uid = 0;
		return false;
	}
	
	/* ○×ボタン入れ替えチェック */
	if( sceUtilityGetSystemParamInt( PSP_SYSTEMPARAM_ID_INT_UNKNOWN,  &ox_swap ) == PSP_SYSTEMPARAM_RETVAL_FAIL ){
		ox_swap = PSP_UTILITY_ACCEPT_CIRCLE;
	}
	if( ox_swap == PSP_UTILITY_ACCEPT_CROSS ){
		st_params->Ctrl.Accept.Button = PSP_CTRL_CROSS;
		st_params->Ctrl.Cancel.Button = PSP_CTRL_CIRCLE;
		st_params->Ctrl.Accept.Symbol = PB_SYM_PSP_CROSS;
		st_params->Ctrl.Cancel.Symbol = PB_SYM_PSP_CIRCLE;
		
		/* ×ボタンが決定の場合、ダイアログの環境フラグをセット */
		cdialogEnable( CDIALOG_ACCEPT_CROSS );
	} else{
		st_params->Ctrl.Accept.Button = PSP_CTRL_CIRCLE;
		st_params->Ctrl.Cancel.Button = PSP_CTRL_CROSS;
		st_params->Ctrl.Accept.Symbol = PB_SYM_PSP_CIRCLE;
		st_params->Ctrl.Cancel.Symbol = PB_SYM_PSP_CROSS;
	}
	/* リピートボタンを設定 */
	padctrlSetRepeatButtons( st_params->Ctrl.Uid,
		PSP_CTRL_UP | PSP_CTRL_RIGHT | PSP_CTRL_DOWN | PSP_CTRL_LEFT |
		st_params->Ctrl.Accept.Button | PSP_CTRL_SQUARE | PSP_CTRL_RTRIGGER | PSP_CTRL_LTRIGGER
	);
	
	/* メッセージを初期化 */
	st_params->CurrentMessage.Main = MF_MM_PROC; /* 最初に送るメッセージ */
	st_params->CurrentMessage.Sub  = 0;
	st_params->NextMessage.Main    = 0;
	st_params->NextMessage.Sub     = 0;
	
	return true;
}

static bool mf_menu_init_graphics( void )
{
	/* pbライブラリ初期化 */
	dbgprint( "Initializing pb..." );
	if( sceDisplayGetFrameBuf( &(st_params->FrameBuffer.OrigAddr), &(st_params->FrameBuffer.Width), &(st_params->FrameBuffer.PixelFormat), PSP_DISPLAY_SETBUF_IMMEDIATE ) != 0 || ! st_params->FrameBuffer.OrigAddr || ! st_params->FrameBuffer.Width ) return false;
	
	pbInit();
	pbEnable( PB_NO_CACHE | PB_BLEND | PB_STEAL_DISPLAY );
	
	/* フレームバッファとして使用するメモリを確保 */
	dbgprint( "Allocating memory for frame buffers..." );
	st_params->FrameBuffer.Size = pbGetFrameBufferDataSize( st_params->FrameBuffer.PixelFormat, st_params->FrameBuffer.Width );
	if( ! mf_alloc_buffers( &(st_params->FrameBuffer), ! st_pref.LowMemoryMode ) ) return false;
	
	/* クリア用背景作成 */
	if( st_params->FrameBuffer.Clear ){
		/* 現在表示されている画面をクリア用背景にコピー */
		sceDmacMemcpy( st_params->FrameBuffer.Clear, st_params->FrameBuffer.OrigAddr, st_params->FrameBuffer.Size );
		
		/* 描画準備 */
		pbSetDisplayBuffer( st_params->FrameBuffer.PixelFormat, st_params->FrameBuffer.Clear, st_params->FrameBuffer.Width );
		pbApply();
		pbFillRect( 0, 0, SCR_WIDTH, SCR_HEIGHT, MF_COLOR_BG );
		mf_draw_frame( false );
	}
	
	/* ダブルバッファリングを有効化 */
	if( st_params->FrameBuffer.Draw ){
		pbSetDrawBuffer( st_params->FrameBuffer.PixelFormat, st_params->FrameBuffer.Draw, st_params->FrameBuffer.Width );
		pbEnable( PB_DOUBLE_BUFFER );
	}
	pbSetDisplayBuffer( st_params->FrameBuffer.PixelFormat, st_params->FrameBuffer.Disp, st_params->FrameBuffer.Width );
	
	return true;
}

void mfMenuMain( SceCtrlData *pad, MfHprmKey *hk )
{
	/* ユーザメモリからメニュー用の作業域を確保 */
	st_params = mfMemoryTempAlloc( sizeof( struct MfMenuParams ) );
	dbgprintf( "Allocating memory for menu: %p: %d bytes", st_params, sizeof( struct MfMenuParams ) );
	if( ! st_params ){
		dbgprint( "Not enough available memory for menu" );
		return;
	}
	memset( st_params, 0, sizeof( struct MfMenuParams ) );
	
	/* サスペンド/リジュームするスレッドの一覧を作成し、サスペンドする */
	if( ! mf_menu_get_target_threads( st_params->Thread.List, &(st_params->Thread.Count) ) ) goto DESTROY;
	mf_menu_change_threads_stat( MF_MENU_SUSPEND_THREADS, st_params->Thread.List, st_params->Thread.Count )
	;
	/* すぐに止まらないスレッドもある? */
	sceDisplayWaitVblankStart();
	
	/* サウンドをミュートにする */
	mf_menu_mute( true );
	
	/* APIがフックされている場合、一時的に元に戻す */
	if( mfIsEnabled() ) mfUnhook();
	
	/* paramsを初期化 */
	if( ! mf_menu_init_params( st_params, pad, hk ) ) goto DESTROY;
	
	/* グラフィックを初期化 */
	if( ! mf_menu_init_graphics() ) goto DESTROY;
	
	/* メニューフラグの初期化 */
	mfMenuEnable( MF_MENU_SCREEN_UPDATE | MF_MENU_EXIT );
	
	/* ルートメニュー設定 */
	st_params->Proc = (MfFuncMenu)mf_root_menu;
	( st_params->Proc )( MF_MM_INIT );
	
	/* 設定したフレームバッファで描画準備 */
	sceDisplayWaitVblankStart();
	pbApply();
	pbSyncDisplay( PB_BUFSYNC_NEXTFRAME );
	
	dbgprint( "Starting menu-loop" );
	while( gRunning ){
		/* 背景を描画 */
		if( st_params->FrameBuffer.Clear ){
			sceDmacMemcpy( pbGetCurrentDrawBufferAddr(), st_params->FrameBuffer.Clear, st_params->FrameBuffer.Size );
			mf_menu_indicator();
		} else if( st_params->FrameBuffer.Draw ){
			memset( pbGetCurrentDrawBufferAddr(), 0, st_params->FrameBuffer.Size );
			mf_draw_frame( false );
			mf_menu_indicator();
		} else{
			SceCtrlData dupe_pad = *(st_params->Ctrl.Pad);
			mfAnalogStickAdjust( &dupe_pad );
			if( ( ( padutilSetPad( dupe_pad.Buttons | padutilGetAnalogStickDirection( dupe_pad.Lx,  dupe_pad.Ly, 0 ) )  ) & MF_HOTKEY_BUTTONS ) || ( st_params->Flags & MF_MENU_SCREEN_UPDATE ) ){
				pbDisable( PB_NO_DRAW );
				mfMenuDisable( MF_MENU_SCREEN_UPDATE );
				memset( pbGetCurrentDrawBufferAddr(), 0, st_params->FrameBuffer.Size );
				mf_draw_frame( true );
				mf_menu_indicator();
			} else{
				pbEnable( PB_NO_DRAW );
			}
		}
		
		padctrlPeekBuffer( st_params->Ctrl.Uid, st_params->Ctrl.Pad, 1 );
		//sceHprmPeekCurrentKey( st_params->Ctrl.HprmKey ); キーリピートが必要
		padutilRemap( st_pref.Remap, padutilSetPad( mfMenuGetCurrentButtons() ), st_params->Ctrl.Pad, st_params->Ctrl.HprmKey, true );
		
#ifdef DEBUG_ENABLED
		mf_debug();
#endif
		
		/*
			MF_MENU_FORCED_QUITがセットされたか、
			MF_MENU_EXITが有効な状態でSTART/HOMEを押下したか、
			次のプロシージャが存在しなければメニューを抜ける。
		*/
		if(
			( st_params->Flags & MF_MENU_FORCED_QUIT ) ||
			( ( st_params->Flags & MF_MENU_EXIT ) && ( st_params->Ctrl.Pad->Buttons & ( PSP_CTRL_START | PSP_CTRL_HOME ) ) ) ||
			! st_params->Proc
		) break;
		
		/* 情報バーをクリア */
		st_params->Info.Text[0] = '\0';
		st_params->Info.Options = 0;
		
		/* 次のメニュープロシージャは今回と同じと仮定 */
		st_params->NextProc = st_params->Proc;
		
		/* ダイアログ状況取得 */
		MfDialogType dialog = mfDialogCurrentType();
		if( dialog ){
			if( st_params->LastDialogType != dialog ){
				st_params->CurrentMessage.Sub |= MF_DM_START;
				mfMenuEnable( MF_MENU_SCREEN_UPDATE );
			} else{
				st_params->CurrentMessage.Sub |= MF_DM_UPDATE;
			}
		} else if( st_params->LastDialogType ){
			st_params->CurrentMessage.Sub |= MF_DM_FINISH;
			mfMenuEnable( MF_MENU_SCREEN_UPDATE );
		}
		st_params->LastDialogType = dialog;
		
		/* プロシージャ実行 */
		( st_params->Proc )( st_params->CurrentMessage.Main | st_params->CurrentMessage.Sub );
		
		/* ダイアログを確定 */
		if( dialog != MF_DIALOG_NONE ){
			mfMenuDrawDialog( dialog, true );
		} else if( st_params->LastDialogType ){
			st_params->LastDialogType = dialog;
			mfMenuResetKeyRepeat();
		} else{
			/* 情報バーを描画 */
			pbPrint(
				pbOffsetChar( 3 ),
				pbOffsetLine( 32 ) - ( st_params->Info.Options & MF_MENU_INFOTEXT_SET_MIDDLE_LINE ? 0 : ( pbOffsetLine( 1 ) >> 1 ) ),
				st_params->Info.Options & MF_MENU_INFOTEXT_ERROR ? MF_COLOR_TEXT_FC : MF_COLOR_TEXT_FG,
				MF_COLOR_TEXT_BG,
				st_params->Info.Text
			);
		}
		
		/* 次のメニューメッセージを確定 */
		if( st_params->Extra.Proc || mfDialogCurrentType() != MF_DIALOG_NONE ){
			st_params->NextMessage.Main = MF_MM_WAIT;
		} else if( st_params->CurrentMessage.Main == MF_MM_WAIT ){
			st_params->NextMessage.Main = MF_MM_CONTINUE;
		} else{
			st_params->NextMessage.Main = MF_MM_PROC;
		}
		
		/* 次のフレームのメッセージを設定 */
		st_params->CurrentMessage.Main = st_params->NextMessage.Main;
		st_params->CurrentMessage.Sub  = st_params->NextMessage.Sub;
		st_params->NextMessage.Main    = 0;
		st_params->NextMessage.Sub     = 0;
		
		/* 次のメニュープロシージャが変更されていれば設定 */
		if( ! st_params->NextProc ){
			dbgprint( "Menu back" );
			( st_params->Proc )( MF_MM_TERM );
			mf_menu_stack( &(st_params->Stack), MF_MENU_STACK_POP, &(st_params->Proc), &(st_params->Menu) );
		} else if( st_params->NextProc != st_params->Proc ){
			dbgprint( "Menu enter" );
			mf_menu_stack( &(st_params->Stack), MF_MENU_STACK_PUSH, st_params->Proc, &(st_params->Menu) );
			st_params->Proc = st_params->NextProc;
			( st_params->Proc )( MF_MM_INIT );
		}
		
		/* 表示バッファと描画バッファを切り替える */
		sceDisplayWaitVblankStart();
		pbSwapBuffers( PB_BUFSYNC_IMMEDIATE );
		
		/* 指定時間待つ */
		if( st_params->WaitMicroSecForUpdate ){
			sceKernelDelayThread( st_params->WaitMicroSecForUpdate );
			st_params->WaitMicroSecForUpdate = 0;
			mfMenuEnable( MF_MENU_SCREEN_UPDATE );
			sceDisplayWaitVblankStart();
		}
	}
	
	pbPrint( pbOffsetChar( 30 ), pbOffsetLine( 17 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_STR_HOME_RETURN_TO_GAME );
	sceDisplayWaitVblankStart();
	pbSwapBuffers( PB_BUFSYNC_NEXTFRAME );
	sceKernelDelayThread( 300000 );
	
	/* メニュースタックに詰まれているメニュー全てにMF_MM_TERMを送る */
	while( st_params->Proc ){
		( st_params->Proc )( MF_MM_TERM );
		mf_menu_stack( &(st_params->Stack), MF_MENU_STACK_POP, &(st_params->Proc), &(st_params->Menu) );
	}
	
	/* ダイアログを終了 */
	mfMenuDrawDialog( mfDialogCurrentType(), false );
	mfDialogFinish();
	
	goto DESTROY;
	
	DESTROY:
		dbgprint( "Destroying menu..." );
		mf_menu_stack( &(st_params->Stack), MF_MENU_STACK_DESTROY, NULL, NULL );
		
		dbgprint( "Memory block return to the system..." );
		padctrlDestroy( st_params->Ctrl.Uid );
		padutilDestroyButtonSymbols();
		
		mf_free_buffers( &(st_params->FrameBuffer) );
		
		/* サウンドを戻す */
		mf_menu_mute( false );
		
		/* メニュー起動前にAPIをフックしていた場合は、フックしなおす */
		if( mfIsEnabled() ) mfHook();
		
		/* 他のスレッドをリジューム */
		dbgprint( "Resuming threads..." );
		sceDisplayWaitVblankStart();
		mf_menu_change_threads_stat( MF_MENU_RESUME_THREADS, st_params->Thread.List, st_params->Thread.Count );
		
		/* スレッドリスト用のメモリを解放 */
		dbgprint( "Menu finish" );
		mfMemoryFree( st_params );
		
		/* ディスプレイを点灯させる */
		sceDisplayWaitVblankStart();
		sceDisplayWaitVblankStart();
		sceDisplayEnable();
}

void mfMenuSetInfoText( unsigned int options, char *format, ... )
{
	va_list ap;
	char *strtop = st_params->Info.Text;
	
	st_params->Info.Options = options;
	
	if( ( options & MF_MENU_INFOTEXT_COMMON_CTRL ) ){
		if( options & MF_MENU_INFOTEXT_MULTICOLUMN_CTRL ){
			strtop += snprintf( strtop, MF_MENU_INFOTEXT_LENGTH, "(%s%s%s%s)%s, ", PB_SYM_PSP_UP, PB_SYM_PSP_RIGHT, PB_SYM_PSP_DOWN, PB_SYM_PSP_LEFT, MF_STR_CTRL_MOVE );
		} else if( options & MF_MENU_INFOTEXT_MOVABLEPAGE_CTRL ){
			strtop += snprintf( strtop, MF_MENU_INFOTEXT_LENGTH, "(%s%s)%s, (%s%s)%s, ", PB_SYM_PSP_UP, PB_SYM_PSP_DOWN, MF_STR_CTRL_MOVE, PB_SYM_PSP_LEFT, PB_SYM_PSP_RIGHT, MF_STR_CTRL_PAGEMOVE );
		} else{
			strtop += snprintf( strtop, MF_MENU_INFOTEXT_LENGTH, "(%s%s)%s, ", PB_SYM_PSP_UP, PB_SYM_PSP_DOWN, MF_STR_CTRL_MOVE );
		}
		
		strtop += snprintf( strtop, MF_MENU_INFOTEXT_LENGTH, "(%s)%s, (START/HOME)%s", mfMenuCancelSymbol(), MF_STR_CTRL_BACK, MF_STR_CTRL_EXIT );
	}
	
	if( options & MF_MENU_INFOTEXT_SET_LOWER_LINE ){
		*strtop = '\n';
		strtop++;
	}
	
	va_start( ap, format );
	vsnprintf( strtop, MF_MENU_INFOTEXT_LENGTH - (unsigned int)( strtop - st_params->Info.Text ), format, ap );
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
		
		allocsize += sizeof( MfMenuEntry * ) * tablecells[i].rows + sizeof( MfMenuEntry ) * tablecells[i].cols;
		if( tablecells[i].cols > 1 ) allocsize += sizeof( unsigned int ) * tablecells[i].cols;
	}
	va_end( ap );
	
	dbgprintf( "Allocating memory for menu-tables: %d bytes", allocsize );
	table = (MfMenuTable *)mfMemoryTempAlloc( allocsize );
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
	mfMemoryFree( table );
}

bool mf_control_engine( MfMessage message, const char *label, void *var, void *unused, void *ex )
{
	bool prev_stat = *((bool *)var);
	bool ret = mfCtrlDefBool( message, label, var, unused, ex );
	
	if( prev_stat != *((bool *)var) ){
		if( *((bool *)var) ){
			mfEnable();
		} else{
			mfDisable();
		}
	}
	
	return ret;
}

static void mf_menu_mute( bool mute )
{
	unsigned int k1 = pspSdkSetK1( 0 );
	
	if( mute ){
		sceSysregAudioIoDisable( 0 );
	} else{
		sceSysregAudioIoEnable( 0 );
	}
	
	pspSdkSetK1( k1 );
}

static void mf_menu_indicator( void )
{
	static char lst_working_indicator[] = "|/-\\";
	static int  lst_working_indicator_index = 0;
	
	if( lst_working_indicator[lst_working_indicator_index] == '\0' ) lst_working_indicator_index = 0;
	
	pbPutChar( pbOffsetChar( 1 ), pbOffsetLine( 1 ), MF_COLOR_TITLE, MF_COLOR_TRANSPARENT, lst_working_indicator[lst_working_indicator_index++] );
}

static bool mf_menu_stack( struct MfMenuStack *stack, enum MfMenuStackAction ctrl, void *menu, struct MfMenuPosition *pos )
{
	switch( ctrl ){
		case MF_MENU_STACK_CREATE:
			stack->Index = 0;
			stack->Size  = MF_MENU_STACK_SIZE( MF_MENU_STACK_NUM );
			stack->Data  = (struct MfMenuStackData *)mfMemoryTempAlloc( stack->Size );
			dbgprintf( "Allocating memory for menu-stack: %p: %d bytes", stack->Data, stack->Size );
			return stack->Data ? true : false;
		case MF_MENU_STACK_PUSH:
			if( stack->Index >= MF_MENU_STACK_NUM ){
				void *old_stack = stack->Data;
				stack->Size += MF_MENU_STACK_SIZE( MF_MENU_STACK_NUM );
				stack->Data = (struct MfMenuStackData *)mfMemoryRealloc( stack->Data, stack->Size );
				dbgprintf( "Re-allocating memory for menu-stack: %p: %d bytes", stack->Data, stack->Size );
				if( ! stack->Data ){
					dbgprint( "Failed to re-allocate memory" );
					stack->Data = old_stack;
					return false;
				} else{
					mfMemoryFree( old_stack );
				}
			}
			stack->Data[stack->Index].Proc = menu;
			if( pos ) stack->Data[stack->Index].Pos = *pos;
			dbgprintf( "Menu address push to menu-stack: %p", stack->Data[stack->Index].Proc );
			stack->Index++;
			return true;
		case MF_MENU_STACK_POP:
			if( stack->Index ){
				stack->Index--;
				dbgprintf( "Menu address pop from menu-stack: %p", stack->Data[stack->Index].Proc );
				*((void **)menu) = stack->Data[stack->Index].Proc;
				if( pos ) *pos = stack->Data[stack->Index].Pos;
				return true;
			} else{
				*((void **)menu) = NULL;
				if( pos ) memset( pos, 0, sizeof( struct MfMenuPosition ) );
				return false;
			}
		case MF_MENU_STACK_DESTROY:
			mfMemoryFree( stack->Data );
			return true;
	}
	return false;
}

static bool mf_menu_get_target_threads( SceUID *thread_id_list, int *thread_id_count )
{
	int i, j;
	
	if( sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, thread_id_list, MF_MENU_MAX_NUMBER_OF_THREADS, thread_id_count ) < 0 ){
		dbgprint( "Failed to get a thread-id list" );
		return false;
	}
	
	for( i = 0; i < *thread_id_count; i++ ){
		for( j = 0; j < st_systhread.Count; j++ ){
			if( thread_id_list[i] == st_systhread.List[j] ) thread_id_list[i] = 0;
		}
	}
	
	return true;
}

static void mf_menu_change_threads_stat( bool stat, SceUID *thread_id_list, int thread_id_count )
{
	int ( *func )( SceUID ) = NULL;
	int i;
	SceUID notific_id = mfNotificationThreadId();
	
	if( stat == MF_MENU_SUSPEND_THREADS ){
		dbgprint( "Get suspend request for all-Threads" );
		func = sceKernelSuspendThread;
	} else if( stat == MF_MENU_RESUME_THREADS ){
		dbgprint( "Get resume request for all-Threads" );
		func = sceKernelResumeThread;
	}
	
	if( ! func || ! thread_id_list || ! thread_id_count ) return;
	
	dbgprint( "Controling all-threads without notification thread..." );
	for( i = 0; i < thread_id_count; i++ ){
		if( ! thread_id_list[i] || thread_id_list[i] == notific_id ) continue;
		( func )( thread_id_list[i] );
	}
	
	if( notific_id ) mfNotificationPrintTerm();
}

static bool mf_alloc_buffers( struct MfMenuFrameBuffer *fb, bool graphics_mem )
{
	unsigned char frame_num = graphics_mem ? sceKernelMaxFreeMemSize() / fb->Size : 0;
	
	fb->VramBase  = (void *)( (unsigned int)sceGeEdramGetAddr() | PB_DISABLE_CACHE );
	
	dbgprintf( "Detect current display buffer addr: %p", fb->OrigAddr );
	
	if( frame_num >= 3 ){
		fb->Vram = mfMemoryTempAlloc( fb->Size * 3 );
		if( ! fb->Vram ) return false;
		
		dbgprintf( "Memory allocated: %d bytes", fb->Size * 3 );
		sceDmacMemcpy( fb->Vram, fb->VramBase, fb->Size * 3 );
		fb->Disp  = fb->VramBase;
		fb->Draw  = (void *)( (unsigned int)(fb->Disp) + fb->Size );
		fb->Clear = (void *)( (unsigned int)(fb->Draw) + fb->Size );
	} else if( frame_num >= 2 ){
		fb->Disp = fb->OrigAddr;
		fb->Draw = mfMemoryTempAlign( 16, fb->Size * 2 );
		if( ! fb->Draw ) return false;
		dbgprintf( "Memory allocated for draw/clear buffers: %d bytes", fb->Size *2 );
		fb->Clear = (void *)( (unsigned int)(fb->Draw) + fb->Size );
	} else if( frame_num >= 1 ){
		fb->Disp = fb->OrigAddr;
		fb->Draw = mfMemoryTempAlign( 16, fb->Size );
		if( ! fb->Draw ) return false;
		dbgprintf( "Memory allocated for draw buffer: %d bytes", fb->Size );
	} else{
		dbgprint( "Not enough available memory: only use current display buffer" );
		fb->Disp = fb->OrigAddr;
	}
	
	return true;
}

static void mf_free_buffers( struct MfMenuFrameBuffer *fb )
{
	sceDisplayWaitVblankStart();
	pbReturnDisplay( PB_BUFSYNC_NEXTFRAME );
	
	if( fb->Vram ){
		dbgprint( "Restore vram buffer" );
		sceDmacMemcpy( fb->VramBase, fb->Vram, fb->Size * 3 );
	} else if( fb->Clear ){
		sceDmacMemcpy( fb->OrigAddr, fb->Clear, fb->Size );
	}
	
	dbgprint( "Free memory for frame buffers" );
	if( fb->Vram ){
		mfMemoryFree( fb->Vram );
	} else if( fb->Draw ){
		mfMemoryFree( fb->Draw );
	}
}

static void mf_draw_frame( bool lowmem )
{
	/* これらのセクション名は、内容が同じである場合、ポインタレベルで同じである */
	const char *targid = mfGetGameId();
	const char *loadid = mfGetIniSection();
	
	pbFillRect( 0, 0, SCR_WIDTH, pbOffsetLine( 3 ), MF_COLOR_TITLE_BAR );
	pbFillRect( 0, SCR_HEIGHT - pbOffsetLine( 3 ), SCR_WIDTH, SCR_HEIGHT, MF_COLOR_INFO_BAR );
	pbPrintf( pbOffsetChar( 3 ), pbOffsetLine( 1 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_TITLE, MF_VERSION );
	pbPrint( pbOffsetChar( 47 ), SCR_HEIGHT - pbOffsetLine( 1 ), ( MF_COLOR_TEXT_FG & 0x00ffffff ) | 0x33000000, MF_COLOR_TEXT_BG, MF_AUTHOR );
	
	pbPrintf( pbOffsetChar( 33 ), pbOffsetLine( 1 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "[ID: %s", targid );
	if( targid == loadid ){
		pbPrint( pbOffsetChar( 33 + 5 + strlen( targid ) ), pbOffsetLine( 1 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "]" );
	} else{
		pbPrintf( pbOffsetChar( 33 + 5 + strlen( targid ) ), pbOffsetLine( 1 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, " / %s: %s]", MF_STR_HOME_LOADED, loadid );
	}
	
	if( lowmem ) pbPrintf( 0, 0, MF_COLOR_EX4, MF_COLOR_TEXT_BG, "<%s>", MF_STR_HOME_LOW_MEMORY_MODE );
	
#ifdef MF_WITH_EXCEPTION_HANDLER
		pbPrint( SCR_WIDTH - pbOffsetChar( 12 ), 0, MF_COLOR_EX4, MF_COLOR_TEXT_BG, "<for DEBUG>" );
#endif
}

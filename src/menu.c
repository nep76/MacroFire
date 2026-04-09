/*
	menu.c
*/

#include "menu.h"
#include "mftable.h"

static int  st_wait        = 0;
static bool st_clear_vsync = false;
static bool st_mfmenu_quit = false;
static SCE_CTRL_LATCH_FUNC st_f_ctrlReadLatch;
static SCE_CTRL_DATA_FUNC  st_f_ctrlPeekBufferPositive;
static SceCtrlLatch        *st_pad_latch;
static SceCtrlData         *st_pad_data;

bool mfMenuInit( void )
{
	return true;
}

void mfMenuTerm( void )
{
}

void mfMenuSetup( SCE_CTRL_LATCH_FUNC ctrlReadLatch, SCE_CTRL_DATA_FUNC ctrlPeekBufferPositive, SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	st_f_ctrlReadLatch          = ctrlReadLatch;
	st_f_ctrlPeekBufferPositive = ctrlPeekBufferPositive;
	st_pad_latch                = pad_latch;
	st_pad_data                 = pad_data;
}

void mfClearColor( u32 color )
{
	blitFillRect( 0, 0, 480, 272, color );
}

void mfMenuQuit( void )
{
	st_mfmenu_quit = true;
}

void mfMenu( void )
{
	SceUID thlist[MAX_NUMBER_OF_THREADS];
	int current_number_of_threads, selected = 0;
	
	MfFuncMenu function = NULL;
	
	MfMenuItem *miList;
	int miCount, i;
	
	sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, thlist, MAX_NUMBER_OF_THREADS, &current_number_of_threads );
	
	mfThreadsStatChange( false, thlist, current_number_of_threads );
	
	miCount = mftableMenuEntry + 2;
	miList  = (MfMenuItem *)MemSceMallocEx( 16, PSP_MEMPART_KERNEL_1, "miList", PSP_SMEM_Low, sizeof( MfMenuItem ) * miCount, 0 );
	
	miList[0].type     = MT_OPTION;
	miList[0].selected = &gMfEngine;
	miList[0].label    = "MacroFire Engine";
	miList[0].value[0] = "OFF";
	miList[0].value[1] = "ON";
	miList[0].value[2] = NULL;
	miList[1].type     = MT_BORDER;
	miList[1].selected = NULL;
	miList[1].label    = NULL;
	miList[1].value[0] = NULL;
	for( i = 2; i < miCount; i++ ){
		miList[i].type     = MT_ANCHOR;
		miList[i].selected = NULL;
		miList[i].label    = mftableMenu[i - 2].label;
		miList[i].value[0] = NULL;
	}
	
	mfClearColor( MENU_BGCOLOR );
	
	while( gRunning ){
		( st_f_ctrlReadLatch )( st_pad_latch );
		( st_f_ctrlPeekBufferPositive )( st_pad_data, 1 );
		
		sceDisplayWaitVblankStart();
		
		blitString( 0, 0, MF_MENU_TOP_MESSAGE, MENU_FGCOLOR, MENU_BGCOLOR );
		
		if(
			st_mfmenu_quit ||
			st_pad_latch->uiMake & PSP_CTRL_START ||
			st_pad_latch->uiMake & PSP_CTRL_HOME ||
			( ! function && st_pad_latch->uiMake & PSP_CTRL_CROSS )
		){
			st_mfmenu_quit = false;
			mfClearColor( 0 );
			break;
		}
		
		if( ! function ){
			switch( mfMenuVertical( 5, 32, miList, miCount, &selected ) ){
				case MR_STAY:
					blitString( 3, 245, "UP = MoveUp, DOWN = MoveDown, CIRCLE = Enter or Change toggle\nCROSS or START = Exit", MENU_FGCOLOR, MENU_BGCOLOR );
				case MR_BACK:
					break;
				case MR_ENTER:
					mfClearColor( MENU_BGCOLOR );
					function = mftableMenu[selected - 2].func;
					break;
			}
		} else{
			if( ! ( function )( st_pad_latch, st_pad_data, mftableMenu[selected - 2].arg ) ){
				mfClearColor( MENU_BGCOLOR );
				function = NULL;
			}
		}
		
		sceKernelDcacheWritebackAll();
		
		if( st_wait ){
			sceKernelDelayThread( st_wait * 1000000 );
			st_wait = 0;
		}
		if( st_clear_vsync ){
			mfClearColor( MENU_BGCOLOR );
			st_clear_vsync = false;
		}
	}
	
	mfThreadsStatChange( true, thlist, current_number_of_threads );
}

void mfMenuMoveFocus( MfMenuMoveFocusDir mvdir, MfMenuItem mi[], size_t items_num, int *selected )
{
	int i;
	
	if( mvdir == MM_PREV ){
		for( i = 1; *selected - i >= 0; i++ ){
			if( mi[*selected - i].label != 0 ){
				*selected -= i;
				break;
			}
		}
	} else if( mvdir == MM_NEXT ){
		for( i = 1; *selected + i < items_num; i++ ){
			if( mi[*selected + i].label != 0 ){
				*selected += i;
				break;
			}
		}
	}
}

MfMenuReturnCode mfMenuVertical( int x, int y, MfMenuItem mi[], size_t items_num, int *selected )
{
	int i;
	char line[MENUITEM_LENGTH];
	
	if( *selected < 0 ) return MR_STAY;
	
	if( st_pad_latch->uiMake & PSP_CTRL_UP ){
		mfMenuMoveFocus( MM_PREV, mi, items_num, selected );
	} else if( st_pad_latch->uiMake & PSP_CTRL_DOWN ){
		mfMenuMoveFocus( MM_NEXT, mi, items_num, selected );
	} else if( st_pad_latch->uiMake & PSP_CTRL_CIRCLE ){
		if( mi[*selected].type == MT_OPTION ){
			if( mi[*selected].value[(*mi[*selected].selected) + 1] != 0 ){
				(*mi[*selected].selected) += 1;
			} else{
				(*mi[*selected].selected) = 0;
			}
			blitFillRect( 0, y + ( *selected * 8 ), 480, 8, MENU_BGCOLOR );
		} else{
			return MR_ENTER;
		}
	} else if( st_pad_latch->uiMake & PSP_CTRL_CROSS ){
		return MR_BACK;
	}
	
	for( i = 0; i < items_num; i++ ){
		if( mi[i].type != MT_BORDER ){
			safe_strncpy( line, mi[i].label, MENUITEM_LENGTH );
			if( mi[i].type == MT_OPTION ){
				safe_strncat( line, ": ", MENUITEM_LENGTH );
				safe_strncat( line, mi[i].value[(*mi[i].selected)], MENUITEM_LENGTH );
			}
			
			if( i == *selected ){
				blitString( x, y, line, MENU_FCCOLOR, MENU_BGCOLOR );
			} else{
				blitString( x, y, line, MENU_FGCOLOR, MENU_BGCOLOR );
			}
		}
		y += 8;
	}
	return MR_STAY;
}

void mfThreadsStatChange( bool stat, SceUID thlist[], int thnum )
{
	int ( *request_stat_func )( SceUID ) = NULL;
	int i;
	
	if( stat ){
		request_stat_func = sceKernelResumeThread;
	} else{
		request_stat_func = sceKernelSuspendThread;
	}
	
	for( i = 0; i < thnum; i++ ){
		if( thlist[i] != sceKernelGetThreadId() ){
			( *request_stat_func )( thlist[i] );
		}
	}
}

void mfWaitScreenReload( int sec )
{
	st_wait = sec;
}

void mfClearScreenWhenVsync( void )
{
	st_clear_vsync = true;
}

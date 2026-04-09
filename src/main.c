/*
	MacroFire
*/

#include "main.h"
#include "hooktable.h"

#define RUN_IN_KERNEL_MODE
PSP_MODULE_INFO( "MacroFire", PSP_MODULE_KERNEL, 0, 0 );

static SceCtrlLatch pad_latch;
static SceCtrlData  pad_data;
static SCE_CTRL_LATCH_FUNC ctrlGetPadLatch = sceCtrlReadLatch;
static SCE_CTRL_DATA_FUNC  ctrlGetPadData  = sceCtrlPeekBufferPositive;

static bool st_apihooked = false;
static unsigned int st_prev_pad_buttons;

static void mfKeyHook( HookCaller caller, SceCtrlData *pad )
{
	int i;
	
	for( i = 0; i < mftableEntry; i++ ){
		if( mftable[i].hook.func ) ( mftable[i].hook.func )( caller, pad, mftable[i].hook.arg );
	}
}

int mfCtrlPeekBufferPositive( SceCtrlData *pad, int count )
{
	int ret = ( (SCE_CTRL_DATA_FUNC)Hooktable[0].syscall.func )( pad, count );
	
	mfKeyHook( CALL_PEEK_BUFFER_POSITIVE, pad );
	
	return ret;
}

int mfCtrlPeekBufferNegative( SceCtrlData *pad, int count )
{
	int ret = ( (SCE_CTRL_DATA_FUNC)Hooktable[1].syscall.func )( pad, count );
	
	mfKeyHook( CALL_PEEK_BUFFER_NEGATIVE, pad );
	
	return ret;
}

int mfCtrlReadBufferPositive( SceCtrlData *pad, int count )
{
	int ret = ( (SCE_CTRL_DATA_FUNC)Hooktable[2].syscall.func )( pad, count );
	
	mfKeyHook( CALL_READ_BUFFER_POSITIVE, pad );
	
	return ret;
}

int mfCtrlReadBufferNegative( SceCtrlData *pad, int count )
{
	int ret = ( (SCE_CTRL_DATA_FUNC)Hooktable[3].syscall.func )( pad, count );
	
	mfKeyHook( CALL_READ_BUFFER_NEGATIVE, pad );
	
	return ret;
}

int mfCtrlPeekLatch( SceCtrlLatch *latch )
{
	SceCtrlData pad;
	int ret = ( (SCE_CTRL_DATA_FUNC)Hooktable[0].syscall.func )( &pad, 1 ); // 0x80000023が返る。なぜ？
	mfKeyHook( CALL_PEEK_LATCH, &pad );
	
	latch->uiMake    |= ( st_prev_pad_buttons ^ pad.Buttons ) & pad.Buttons;
	latch->uiBreak   |= ( st_prev_pad_buttons ^ pad.Buttons ) & st_prev_pad_buttons;
	latch->uiPress   |= pad.Buttons;
	latch->uiRelease = ~0;
	
	st_prev_pad_buttons = pad.Buttons;
	
	return ret;
}

int mfCtrlReadLatch( SceCtrlLatch *latch )
{
	SceCtrlData pad;
	int ret = ( (SCE_CTRL_DATA_FUNC)Hooktable[0].syscall.func )( &pad, 1 ); // 0x80000023が返る。なぜ？
	mfKeyHook( CALL_READ_LATCH, &pad );
	
	latch->uiMake    = ( st_prev_pad_buttons ^ pad.Buttons ) & pad.Buttons;
	latch->uiBreak   = ( st_prev_pad_buttons ^ pad.Buttons ) & st_prev_pad_buttons;
	latch->uiPress   = pad.Buttons;
	latch->uiRelease = ~pad.Buttons;
	
	st_prev_pad_buttons = pad.Buttons;
	
	return ret;
}

bool mfIsApiHooked( void )
{
	return st_apihooked;
}

void mfHookApi( void )
{
	int i;
	
	if( st_apihooked ) return;
	
	for( i = 0; i < ARRAY_NUM( Hooktable ); i++ ){
		hookApiByNid(
			&(Hooktable[i].syscall),
			Hooktable[i].modname,
			Hooktable[i].library,
			Hooktable[i].nid,
			Hooktable[i].hookfunc
		);
	}
	st_apihooked = true;
}

void mfRestoreApi( void )
{
	int i;
	
	if( ! st_apihooked ) return;
	
	for( i = 0; i < ARRAY_NUM( Hooktable ); i++ ){
		hookRestoreAddr( &(Hooktable[i].syscall) );
	}
	st_apihooked = false;
}

int main_thread( SceSize arglen, void *argp )
{	
	/* メニューを初期化 */
	mfMenuInit();
	mfMenuSetup( ctrlGetPadLatch, ctrlGetPadData, &pad_latch, &pad_data );
	
	{
		chdir( "ms0:" );
		
		/* 初期化関数呼び出し */
		int i;
		for( i = 0; i < mftableEntry; i++ ){
			if( mftable[i].initFunc ) ( mftable[i].initFunc )();
		}
	}
	
	while( gRunning ){
		sceKernelDelayThread( 50000 );
		
		if( ! st_apihooked && gMfEngine ){
			// APIをフック
			mfHookApi();
		} else if( st_apihooked && ! gMfEngine ){
			mfRestoreApi();
		}
		
		( ctrlGetPadData )( &pad_data, 1 );
		( ctrlGetPadLatch )( &pad_latch );
		
		if( pad_data.Buttons & PSP_CTRL_VOLUP && pad_data.Buttons & PSP_CTRL_VOLDOWN ) mfMenu();
	}
	
	/* メニュー終了処理 */
	mfMenuTerm();
	
	{
		/* 終了関数呼び出し */
		int i;
		for( i = 0; i < mftableEntry; i++ ){
			if( mftable[i].termFunc ) ( mftable[i].termFunc )();
		}
	}
	
	return sceKernelExitDeleteThread( 0 );
}


int module_start( SceSize arglen, void *argp )
{
	SceUID thid;
	
	thid = sceKernelCreateThread( "MacroFire", main_thread, 15, 0x1500, PSP_THREAD_ATTR_NO_FILLSTACK, 0 );
	if( thid ) sceKernelStartThread( thid, arglen, argp );
	
	return 0;
}

int module_stop( void )
{
	int i;
	gRunning = false;
	
	for( i = 0; i < ARRAY_NUM( Hooktable ); i++ ){
		hookRestoreAddr( &(Hooktable[i].syscall) );
	}
	
	return 0;
}

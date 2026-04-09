/*
	msg.c
*/

#include "msg.h"

static u32 fgcolor = 0xffffffff;
static u32 bgcolor = 0xff000000;
static u32 fccolor = 0xff0000ff;

static void cmndlg_msg_measure_max_width( char *str, int *width, int *br )
{
	int cur_width = 0;
	char *start = str;
	char *end   = NULL;
	
	*width = 0;
	*br    = 0;
	
	while( ( end = strchr( start, '\n' ) ) ){
		*br += 1;
		cur_width = end - start;
		
		if( cur_width > *width ) *width = cur_width;
		
		start = end + 1;
	}
	
	if( ! *width ) *width = strlen( str );
}

CmndlgMsgRc cmndlgMsg( unsigned int x, unsigned int y, CmndlgMsgOption opt, char *str )
{
	int max_width, lines;
	bool yesno = ( opt & CMNDLG_MSG_OPTION_YESNO ? true : false );
	char *btn;
	CmndlgMsgRc rc = CMNDLG_MSG_YES;
	SceCtrlLatch pad_latch;
	
	cmndlg_msg_measure_max_width( str, &max_width, &lines );
	
	if( yesno ){
		btn = "  \x85 = YES  \x86 = NO  ";
	} else{
		btn = "  \x85 = OK  ";
	}
	
	if( max_width < strlen( btn ) ) max_width = strlen( btn );
	
	blitFillRect( CMNDLG_MSG_X( x, 0 ), CMNDLG_MSG_Y( y, 0 ), CMNDLG_MSG_X( x, max_width + 2 ), CMNDLG_MSG_Y( y, lines + 5 ), bgcolor );
	blitLineRect( CMNDLG_MSG_X( x, 0 ), CMNDLG_MSG_Y( y, 0 ), CMNDLG_MSG_X( x, max_width + 2 ), CMNDLG_MSG_Y( y, lines + 5 ), fgcolor );
	blitString( CMNDLG_MSG_X( x, 1 ), CMNDLG_MSG_Y( y, 1 ), fgcolor, bgcolor, str );
	
	/* ƒ{ƒ^ƒ““ü—Í‚ð–³Ž‹‚·‚éŽžŠÔ */
	sceKernelDelayThread( 400000 );
	
	blitString( CMNDLG_MSG_X( x, 1 ), CMNDLG_MSG_Y( y, lines + 3 ), fccolor, bgcolor, btn );
	
	sceCtrlReadLatch( &pad_latch );
	for( ;; ){
		sceCtrlReadLatch( &pad_latch );
		
		if( pad_latch.uiMake & PSP_CTRL_CIRCLE ){
			rc = CMNDLG_MSG_YES;
			break;
		} else if( pad_latch.uiMake & PSP_CTRL_CROSS && yesno ){
			rc = CMNDLG_MSG_NO;
			break;
		}
		
		sceDisplayWaitVblankStart();
	}
	
	return rc;
}

CmndlgMsgRc cmndlgMsgf( unsigned int x, unsigned int y, CmndlgMsgOption opt, char *format, ... )
{
	char str[255];
	va_list ap;
	
	va_start( ap, format );
	vsnprintf( str, sizeof( str ), format, ap );
	va_end( ap );
	
	return cmndlgMsg( x, y, opt, str );
}

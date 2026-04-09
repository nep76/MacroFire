/*
	getdigits.c
*/

#include "getbuttons.h"

#define CMNDLG_GETBUTTONS_ALL_AVAIL_BUTTONS 16
static struct cmndlg_getbuttons_buttonstat st_btnstat[CMNDLG_GETBUTTONS_ALL_AVAIL_BUTTONS] = {
	{ PSP_CTRL_CIRCLE,   false },
	{ PSP_CTRL_CROSS,    false },
	{ PSP_CTRL_SQUARE,   false },
	{ PSP_CTRL_TRIANGLE, false },
	{ PSP_CTRL_UP,       false },
	{ PSP_CTRL_RIGHT,    false },
	{ PSP_CTRL_DOWN,     false },
	{ PSP_CTRL_LEFT,     false },
	{ PSP_CTRL_LTRIGGER, false },
	{ PSP_CTRL_RTRIGGER, false },
	{ PSP_CTRL_SELECT,   false },
	{ PSP_CTRL_START,    false },
	{ PSP_CTRL_NOTE,     false },
	{ PSP_CTRL_SCREEN,   false },
	{ PSP_CTRL_VOLUP,    false },
	{ PSP_CTRL_VOLDOWN,  false }
};

static void cmndlg_getbuttons_move_prev( unsigned int mask, int *selected )
{
	int i;
	
	if( ! selected ) return;
	
	for( i = *selected - 1; i >= 0; i-- ){
		if( ! ( mask & st_btnstat[i].button ) ){
			*selected = i;
			break;
		}
	}
}

static void cmndlg_getbuttons_move_next( unsigned int mask, int *selected )
{
	int i;
	
	if( ! selected ) return;
	
	for( i = *selected + 1; i < CMNDLG_GETBUTTONS_ALL_AVAIL_BUTTONS; i++ ){
		if( ! ( mask & st_btnstat[i].button ) ){
			*selected = i;
			break;
		}
	}
}

static void cmndlg_getbuttons_set_buttons( unsigned int buttons )
{
	int i;
	for( i = 0; i < CMNDLG_GETBUTTONS_ALL_AVAIL_BUTTONS; i++ ){
		if( buttons & st_btnstat[i].button ){
			st_btnstat[i].stat = true;
		} else{
			st_btnstat[i].stat = false;
		}
	}
}

static unsigned int cmndlg_getbuttons_get_buttons( void )
{
	unsigned int buttons = 0;
	int i;
	for( i = 0; i < CMNDLG_GETBUTTONS_ALL_AVAIL_BUTTONS; i++ ){
		if( st_btnstat[i].stat ) buttons |= st_btnstat[i].button;
	}
	return buttons;
}

static void cmndlg_getbuttons_drawdialog( unsigned int x, unsigned int y, u32 fgcolor, u32 bgcolor, CmndlgGetButtons cgb[], unsigned int target, unsigned int count )
{
	if( ! cgb || ! count ) return;
	
	int i, lines = 0;
	for( i = 0; i < CMNDLG_GETBUTTONS_ALL_AVAIL_BUTTONS; i++ ){
		if( ! ( cgb[target].btnMask & st_btnstat[i].button ) ) lines++;
	}
	
	if( count > 1 ){
		int y_offset = 7 + lines;
		
		blitFillBox( x, y, blitMeasureChar( 30 ), blitMeasureLine( 9 + lines ), bgcolor );
		blitLineBox( x, y, blitMeasureChar( 30 ), blitMeasureLine( 9 + lines ), fgcolor );
		
		if( target ){
			blitStringf(
				x + 2,
				y + BLIT_CHAR_HEIGHT * y_offset++,
				fgcolor,
				bgcolor,
				"L = %s",
				cgb[target - 1].title
			);
		}
		
		if( target < count - 1 ){
			blitStringf(
				x + 2,
				y + BLIT_CHAR_HEIGHT * y_offset,
				fgcolor,
				bgcolor,
				"R = %s",
				cgb[target + 1].title
			);
		}
	} else{
		blitFillBox( x, y, blitMeasureChar( 30 ), blitMeasureLine( 7 + lines ), bgcolor );
		blitLineBox( x, y, blitMeasureChar( 30 ), blitMeasureLine( 7 + lines ), fgcolor );
	}
	
	blitString(
		x + BLIT_CHAR_WIDTH,
		y + BLIT_CHAR_HEIGHT,
		fgcolor,
		bgcolor,
		cgb[target].title
	);
	blitString(
		x + BLIT_CHAR_WIDTH,
		y + BLIT_CHAR_HEIGHT * ( 4 + lines ),
		fgcolor,
		bgcolor,
		"\x80\x82 = Move, \x83\x81 = Change value\n\x85 = Accept, \x86 = Cancel"
	);
}

int cmndlgGetButtons( unsigned int x, unsigned int y, CmndlgGetButtons cgb[], unsigned int count )
{
	SceCtrlLatch pad_latch;
	bool accept = false;
	unsigned int i, target = 0;
	struct cmndlg_getbuttons_savestat *savestat;
	
	if( ! cgb || ! count ) return CMNDLG_ERROR_INVALID_ARGUMENTS;
	
	savestat = (struct cmndlg_getbuttons_savestat *)memsceMalloc( sizeof( struct cmndlg_getbuttons_savestat ) * count );
	if( ! savestat ) return CMNDLG_ERROR_FAILED_TO_MEMORY_ALLOCATE;
	
	u32 fgcolor = cmndlgGetFgColor();
	u32 bgcolor = cmndlgGetBgColor();
	u32 fccolor = cmndlgGetFcColor();
	
	/* ‰Šú‰» */
	for( i = 0; i < count; i++ ){
		savestat[i].selected = -1;
		cmndlg_getbuttons_move_next( cgb[i].btnMask, &(savestat[i].selected) );
		savestat[i].buttons = *(cgb[i].buttons);
	}
	
	cmndlg_getbuttons_set_buttons( savestat[target].buttons );
	cmndlg_getbuttons_drawdialog( x, y, fgcolor, bgcolor, cgb, target, count );
	
	for( ;; ){
		int drawline;
		
		sceCtrlReadLatch( &pad_latch );
		
		if( pad_latch.uiMake & PSP_CTRL_CROSS ){
			accept = false;
			break;
		} else if( pad_latch.uiMake & PSP_CTRL_CIRCLE ){
			savestat[target].buttons = cmndlg_getbuttons_get_buttons();
			accept = true;
			break;
		} else if( pad_latch.uiMake & PSP_CTRL_UP ){
			cmndlg_getbuttons_move_prev( cgb[target].btnMask, &(savestat[target].selected) );
		} else if( pad_latch.uiMake & PSP_CTRL_DOWN ){
			cmndlg_getbuttons_move_next( cgb[target].btnMask, &(savestat[target].selected) );
		} else if( pad_latch.uiMake & PSP_CTRL_LEFT || pad_latch.uiMake & PSP_CTRL_RIGHT ){
			st_btnstat[savestat[target].selected].stat = st_btnstat[savestat[target].selected].stat ? false : true;
		} else if( pad_latch.uiMake & PSP_CTRL_LTRIGGER && target ){
			savestat[target].buttons = cmndlg_getbuttons_get_buttons();
			target--;
			cmndlg_getbuttons_set_buttons( savestat[target].buttons );
			cmndlg_getbuttons_drawdialog( x, y, fgcolor, bgcolor, cgb, target, count );
		} else if( pad_latch.uiMake & PSP_CTRL_RTRIGGER && target < count - 1 ){
			savestat[target].buttons = cmndlg_getbuttons_get_buttons();
			target++;
			cmndlg_getbuttons_set_buttons( savestat[target].buttons );
			cmndlg_getbuttons_drawdialog( x, y, fgcolor, bgcolor, cgb, target, count );
		}
		
		for( i = 0, drawline = 0; i < CMNDLG_GETBUTTONS_ALL_AVAIL_BUTTONS; i++ ){
			char item[32];
			if( cgb[target].btnMask & st_btnstat[i].button ) continue;
			
			if     ( st_btnstat[i].button == PSP_CTRL_CIRCLE   ){ strcpy( item, "Circle       : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_CROSS    ){ strcpy( item, "Cross        : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_SQUARE   ){ strcpy( item, "Square       : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_TRIANGLE ){ strcpy( item, "Triangle     : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_UP       ){ strcpy( item, "Up           : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_RIGHT    ){ strcpy( item, "Right        : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_DOWN     ){ strcpy( item, "Down         : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_LEFT     ){ strcpy( item, "Left         : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_LTRIGGER ){ strcpy( item, "L-trig       : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_RTRIGGER ){ strcpy( item, "R-trig       : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_SELECT   ){ strcpy( item, "SELECT       : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_START    ){ strcpy( item, "START        : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_NOTE     ){ strcpy( item, "MusicNote(\x88) : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_SCREEN   ){ strcpy( item, "Blightness   : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_VOLUP    ){ strcpy( item, "VolumeUp(+)  : " ); }
			else if( st_btnstat[i].button == PSP_CTRL_VOLDOWN  ){ strcpy( item, "VolumeDown(-): " ); }
			
			if( st_btnstat[i].stat ){
				strcat( item, "ON " );
			} else{
				strcat( item, "OFF" );
			}
			
			if( i == savestat[target].selected ){
				blitString(
					x + BLIT_CHAR_WIDTH,
					y + BLIT_CHAR_HEIGHT * ( drawline + 3 ),
					fccolor,
					bgcolor,
					item
				);
			} else{
				blitString(
					x + BLIT_CHAR_WIDTH,
					y + BLIT_CHAR_HEIGHT * ( drawline + 3 ),
					fgcolor,
					bgcolor,
					item
				);
			}
			
			drawline++;
		}
		
		sceDisplayWaitVblankStart();
		sceKernelDcacheWritebackAll();
	}
	
	/* “ü—Í’l‚ð”½‰f */
	if( accept ){
		for( i = 0; i < count; i++ ){
			*(cgb[i].buttons) = savestat[i].buttons;
		}
	}
	
	memsceFree( savestat );
	return CMNDLG_ERROR_SUCCESS;
}

/*
	getdigits.c
*/

#include "getdigits.h"

static void cmndlg_getdigits_drawdialog( unsigned int x, unsigned int y, u32 fgcolor, u32 bgcolor, CmndlgGetDigits cgd[], unsigned int target, unsigned int count )
{
	if( ! cgd || ! count ) return;
	
	if( count > 1 ){
		int y_offset = 9;
		
		blitFillBox( x, y, blitMeasureChar( 30 ), blitMeasureLine( 11 ), bgcolor );
		blitLineBox( x, y, blitMeasureChar( 30 ), blitMeasureLine( 11 ), fgcolor );
		
		if( target ){
			blitStringf(
				x + 2,
				y + BLIT_CHAR_HEIGHT * y_offset++,
				fgcolor,
				bgcolor,
				"L = %s",
				cgd[target - 1].title
			);
		}
		
		if( target < count - 1 ){
			blitStringf(
				x + 2,
				y + BLIT_CHAR_HEIGHT * y_offset,
				fgcolor,
				bgcolor,
				"R = %s",
				cgd[target + 1].title
			);
		}
	} else{
		blitFillBox( x, y, blitMeasureChar( 30 ), blitMeasureLine( 9 ), bgcolor );
		blitLineBox( x, y, blitMeasureChar( 30 ), blitMeasureLine( 9 ), fgcolor );
	}
	
	blitString(
		x + BLIT_CHAR_WIDTH,
		y + BLIT_CHAR_HEIGHT,
		fgcolor,
		bgcolor,
		cgd[target].title
	);
	blitString(
		x + BLIT_CHAR_WIDTH,
		y + BLIT_CHAR_HEIGHT * 6,
		fgcolor,
		bgcolor,
		"\x83\x81 = Move, \x80\x82 = Change value\n\x85 = Accept, \x86 = Cancel"
	);
}

int cmndlgGetDigits( unsigned int x, unsigned int y, CmndlgGetDigits cgd[], unsigned int count )
{
	SceCtrlLatch pad_latch;
	bool accept = false;
	unsigned int i, target = 0;
	struct cmndlg_getdigits_savestat *savestat;
	
	if( ! cgd || ! count ) return CMNDLG_ERROR_INVALID_ARGUMENTS;
	
	savestat = (struct cmndlg_getdigits_savestat *)memsceMalloc( sizeof( struct cmndlg_getdigits_savestat ) * count );
	if( ! savestat ) return CMNDLG_ERROR_FAILED_TO_MEMORY_ALLOCATE;

	u32 fgcolor = cmndlgGetFgColor();
	u32 bgcolor = cmndlgGetBgColor();
	u32 fccolor = cmndlgGetFcColor();
		
	/* ‰Šú‰» */
	for( i = 0; i < count; i++ ){
		savestat[i].numMaxIndex = cgd[i].numDigits;
		if( savestat[i].numMaxIndex > CMNDLG_GETDIGITS_MAX_DIGIT )
			savestat[i].numMaxIndex = CMNDLG_GETDIGITS_MAX_DIGIT;
		
		if( savestat[i].numMaxIndex <= 0 ){
			savestat[i].numBuf[0]   = '\0';
			savestat[i].numStart    = NULL;
			savestat[i].numMaxIndex = 0;
			savestat[i].selected    = -1;
			continue;
		}
		
		savestat[i].numStart = (savestat[i].numBuf) + ( CMNDLG_GETDIGITS_MAX_DIGIT - savestat[i].numMaxIndex );
		
		snprintf( savestat[i].numBuf, CMNDLG_GETDIGITS_MAX_DIGIT + 1, "%09ld", *(cgd[i].number) );
		savestat[i].selected = savestat[i].numMaxIndex - 1;
	}
	
	cmndlg_getdigits_drawdialog( x, y, fgcolor, bgcolor, cgd, target, count );
	
	for( ;; ){
		sceCtrlReadLatch( &pad_latch );
		
		if( pad_latch.uiMake & PSP_CTRL_CROSS ){
			accept = false;
			break;
		} else if( pad_latch.uiMake & PSP_CTRL_CIRCLE ){
			accept = true;
			break;
		} else if( pad_latch.uiMake & PSP_CTRL_UP ){
			if( savestat[target].numStart[savestat[target].selected] == '9' ){
				savestat[target].numStart[savestat[target].selected] = '0';
			} else{
				savestat[target].numStart[savestat[target].selected]++;
			}
		} else if( pad_latch.uiMake & PSP_CTRL_DOWN ){
			if( savestat[target].numStart[savestat[target].selected] == '0' ){
				savestat[target].numStart[savestat[target].selected] = '9';
			} else{
				savestat[target].numStart[savestat[target].selected]--;
			}
		} else if( pad_latch.uiMake & PSP_CTRL_LEFT && savestat[target].selected ){
			savestat[target].selected--;
		} else if( pad_latch.uiMake & PSP_CTRL_RIGHT && savestat[target].selected < savestat[target].numMaxIndex - 1 ){
			savestat[target].selected++;
		} else if( pad_latch.uiMake & PSP_CTRL_LTRIGGER && target ){
			target--;
			cmndlg_getdigits_drawdialog( x, y, fgcolor, bgcolor, cgd, target, count );
		} else if( pad_latch.uiMake & PSP_CTRL_RTRIGGER && target < count - 1 ){
			target++;
			cmndlg_getdigits_drawdialog( x, y, fgcolor, bgcolor, cgd, target, count );
		}
		
		for( i = 0; savestat[target].numStart[i] != '\0'; i++ ){
			char editnum[6];
			if( i == savestat[target].selected ){
				editnum[0] = '\x80';
				editnum[1] = '\n';
				editnum[2] = savestat[target].numStart[i];
				editnum[3] = '\n';
				editnum[4] = '\x82';
				editnum[5] = '\0';
				blitString(
					x + BLIT_CHAR_WIDTH * ( i + 1 ),
					y + BLIT_CHAR_HEIGHT * 2,
					fccolor,
					bgcolor,
					editnum
				);
			} else{
				editnum[0] = '\x20';
				editnum[1] = '\n';
				editnum[2] = savestat[target].numStart[i];
				editnum[3] = '\n';
				editnum[4] = '\x20';
				editnum[5] = '\0';
				blitString(
					x + BLIT_CHAR_WIDTH * ( i + 1 ),
					y + BLIT_CHAR_HEIGHT * 2,
					fgcolor,
					bgcolor,
					editnum
				);
			}
		}
		
		blitString(
			x + BLIT_CHAR_WIDTH * ( i + 2 ),
			y + BLIT_CHAR_HEIGHT * 3,
			fgcolor,
			bgcolor,
			cgd[target].unit
		);
		
		sceDisplayWaitVblankStart();
		sceKernelDcacheWritebackAll();
	}
	
	/* “ü—Í’l‚ð”½‰f */
	if( accept ){
		for( i = 0; i < count; i++ ){
			*(cgd[i].number) = strtol( savestat[i].numStart, NULL, 10 );
		}
	}
	
	memsceFree( savestat );
	return CMNDLG_ERROR_SUCCESS;
}

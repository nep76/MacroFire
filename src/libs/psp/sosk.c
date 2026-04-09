/*
	sosk.c
*/

#include "sosk.h"

#define UND       0 /* Undefined character */
#define SOSK_ROWS 8
#define SOSK_COLS 13
static char st_charTable[SOSK_ROWS][SOSK_COLS] = {
	{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M' },
	{ 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' },
	{ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm' },
	{ 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' },
	{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '-', '*' },
	{ '!', '?', '@', '#', '$', '%', '&', '<', '=', '>', '~', '^', '/' },
	{ '\"','\'',',', '.', ':', ';', '`', '|', '(', ')', '[', ']', '\\'},
	{ '{', '}', '_', UND, UND, UND, UND, UND, UND, UND, UND, UND, UND }
};

#define SOSK_WIDTH        ( SOSK_COLS * 2 + 12 )
#define SOSK_HEIGHT       ( SOSK_ROWS * 2 + 7  )
#define SOSK_RESULT_WIDTH ( SOSK_WIDTH - 3 )

static void sosk_cursor_forward( int *cursor, int *offset, int max )
{
	*cursor += 1;
	if( *cursor > max ){
		*cursor = max;
		*offset += 1;
	}
}

static void sosk_cursor_back( int *cursor, int *offset )
{
	*cursor -= 1;
	if( *cursor < 0 ){
		*cursor = 0;
		if( *offset ) *offset -= 1;
	}
}

static int sosk_input_add( char *str, int offset, char chr, size_t maxlen )
{
	int length = strlen( str );
	
	if( length + 1 > maxlen ) return length;
	
	if( offset != length ){
		memmove( str + offset + 1, str + offset, length - offset + 1 );
	} else{
		str[offset + 1] = '\0';
	}
	str[offset] = chr;
	
	return length + 1;
}

static int sosk_input_bs( char *str, int offset )
{
	int length = strlen( str );
	
	if( ! length ) return 0;
	
	if( offset != length ){
		memmove( str + offset - 1, str + offset, length - offset + 1 );
	} else{
		str[offset - 1] = '\0';
	}
	
	return length - 1;
}

static int sosk_move_focus( int cp, int mv, int max )
{
	cp += mv;
	if( cp < 0 ){
		cp = max;
	} else if( cp > max ){
		cp = 0;
	}
	
	return cp;
}

SoskRc soskRun(
	char *desc,
	unsigned int x,
	unsigned int y,
	u32 fgcolor,
	u32 bgcolor,
	u32 fccolor,
	char *input,
	size_t maxlen
)
{
	SceCtrlLatch pad_latch;
	int ix = 0, iy = 0, cx = 0, cy = 0, length, cursor, move_x = 0, move_y = 0;
	bool update_result = true;
	bool update_dialog = true;
	int result_offset = 0;
	SoskRc rc = SR_CANCEL;
	char *orig_string;
	
	length = strlen( input );
	if( length > maxlen ){
		input[maxlen - 1] = '\0';
		length = maxlen - 1;
	}
	
	if( length > SOSK_RESULT_WIDTH ){
		result_offset = length - SOSK_RESULT_WIDTH;
		cursor = SOSK_RESULT_WIDTH;
	} else{
		cursor = length;
	}
	
	orig_string = (char *)memsceMalloc( length + 1 );
	if( ! orig_string ) return SR_ERROR;
	strcpy( orig_string, input );
	
	blitFillRect( SOSK_X( x, 0 ), SOSK_Y( y, 0 ), SOSK_X( x, SOSK_WIDTH ), SOSK_Y( y, SOSK_HEIGHT ), bgcolor );
	blitLineRect( SOSK_X( x, 0 ), SOSK_Y( y, 0 ), SOSK_X( x, SOSK_WIDTH ), SOSK_Y( y, SOSK_HEIGHT ), fgcolor );
	blitString( SOSK_X( x, 1 ), SOSK_Y( y, SOSK_ROWS * 2 + 3 ), fgcolor, bgcolor, "\x85 = Input, \x84 = Space, \x87 = Backspace" );
	blitString( SOSK_X( x, 1 ), SOSK_Y( y, SOSK_ROWS * 2 + 4 ), fgcolor, bgcolor, "\x80\x82\x83\x81 = Move, LR = MoveCursor" );
	blitString( SOSK_X( x, 1 ), SOSK_Y( y, SOSK_ROWS * 2 + 5 ), fgcolor, bgcolor, "START = Accept, \x86 = Cancel" );
	
	for( ;; ){
		sceCtrlReadLatch( &pad_latch );
		
		if( update_dialog ){
			blitFillRect( SOSK_X( x, 0 ), SOSK_Y( y, 0 ), SOSK_X( x, SOSK_WIDTH ), SOSK_Y( y, SOSK_HEIGHT ), bgcolor );
			blitLineRect( SOSK_X( x, 0 ), SOSK_Y( y, 0 ), SOSK_X( x, SOSK_WIDTH ), SOSK_Y( y, SOSK_HEIGHT ), fgcolor );
			blitString( SOSK_X( x, 1 ), SOSK_Y( y, SOSK_ROWS * 2 + 3 ), fgcolor, bgcolor, "\x85 = Input, \x84 = Space, \x87 = Backspace" );
			blitString( SOSK_X( x, 1 ), SOSK_Y( y, SOSK_ROWS * 2 + 4 ), fgcolor, bgcolor, "\x80\x82\x83\x81 = Move, LR = MoveCursor" );
			blitString( SOSK_X( x, 1 ), SOSK_Y( y, SOSK_ROWS * 2 + 5 ), fgcolor, bgcolor, "START = Accept, \x86 = Cancel" );
			update_dialog = false;
			update_result = true;
		}
		
		if( update_result ){
			char name[SOSK_RESULT_WIDTH + 1];
			strutilSafeCopy( name, input + result_offset, sizeof( name ) );
			
			blitFillRect( SOSK_X( x, 1 ), SOSK_Y( y, 1 ), SOSK_X( x, sizeof( name ) ), SOSK_Y( y, 2 ), bgcolor );
			blitString( SOSK_X( x, 1 ), SOSK_Y( y, 1 ), fgcolor, bgcolor, name );
			blitLine( SOSK_X( x, 1 + cursor ), SOSK_Y( y, 1 ), SOSK_X( x, 1 + cursor ), SOSK_Y( y, 2 ), fccolor );
			blitStringf( SOSK_X( x, 1 ), SOSK_Y( y, 2 ), fgcolor, bgcolor, "( %d / %d )", length, maxlen );
			update_result = false;
		}
		
		for( iy = 0; iy < SOSK_ROWS; iy++ ){
			for( ix = 0; ix < SOSK_COLS; ix++ ){
				if( ix == cx && iy == cy ){
					blitChar( SOSK_X( x, 6 + ix * 2 ), SOSK_Y( y, 3 + iy * 2 ), st_charTable[iy][ix], fccolor, bgcolor );
				} else{
					blitChar( SOSK_X( x, 6 + ix * 2 ), SOSK_Y( y, 3 + iy * 2 ), st_charTable[iy][ix], fgcolor, bgcolor );
				}
			}
		}
		
		if( pad_latch.uiMake & PSP_CTRL_CIRCLE ){
			if( length >= maxlen ){
				cmndlgMsg( SOSK_X( x, 12 ), SOSK_Y( y, 7 ), 0, "Buffer Full" );
				update_dialog = true;
			} else{
				length = sosk_input_add( input, cursor + result_offset, st_charTable[cy][cx], maxlen );
				if( length < maxlen ) sosk_cursor_forward( &cursor, &result_offset, SOSK_RESULT_WIDTH );
				update_result = true;
			}
		} else if( pad_latch.uiMake & PSP_CTRL_TRIANGLE ){
			length = sosk_input_add( input, cursor + result_offset, '\x20', maxlen );
			if( length < maxlen ) sosk_cursor_forward( &cursor, &result_offset, SOSK_RESULT_WIDTH );
			update_result = true;
		} else if( pad_latch.uiMake & PSP_CTRL_SQUARE && ( length && cursor ) ){
			length = sosk_input_bs( input, cursor + result_offset );
			if( result_offset ){
				result_offset--;
			} else{
				sosk_cursor_back( &cursor, &result_offset );
			}
			update_result = true;
		} else if( pad_latch.uiMake & PSP_CTRL_CROSS || pad_latch.uiMake & PSP_CTRL_HOME ){
			rc = SR_CANCEL;
			strcpy( input, orig_string );
			break;
		} else if( pad_latch.uiMake & PSP_CTRL_START ){
			rc = SR_ACCEPT;
			break;
		} else if( pad_latch.uiMake & PSP_CTRL_UP ){
			move_y = -1;
		} else if( pad_latch.uiMake & PSP_CTRL_DOWN ){
			move_y = +1;
		} else if( pad_latch.uiMake & PSP_CTRL_LEFT ){
			move_x = -1;
		} else if( pad_latch.uiMake & PSP_CTRL_RIGHT ){
			move_x = +1;
		} else if( pad_latch.uiMake & PSP_CTRL_LTRIGGER && ( cursor || result_offset ) ){
			sosk_cursor_back( &cursor, &result_offset );
			update_result = true;
		} else if( pad_latch.uiMake & PSP_CTRL_RTRIGGER && cursor + result_offset < length ){
			sosk_cursor_forward( &cursor, &result_offset, SOSK_RESULT_WIDTH );
			update_result = true;
		}
		
		if( move_y ){
			do{
				cy = sosk_move_focus( cy, move_y, SOSK_ROWS - 1 );
			} while( st_charTable[cy][cx] == UND );
			move_y = 0;
		}
		if( move_x ){
			do{
				cx = sosk_move_focus( cx, move_x, SOSK_COLS - 1 );
			} while( st_charTable[cy][cx] == UND );
			move_x = 0;
		}
	}
	
	memsceFree( orig_string );
	
	return rc;
}

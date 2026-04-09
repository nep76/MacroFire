/*
	macroeditor.c
*/

#include "macroeditor.h"

static unsigned int st_selected_macro_num = 0;
static unsigned int st_macro_all_lines = 0;


#define MACROEDITOR_EDIT_DATA  0
#define MACROEDITOR_CH_TYPE    1
#define MACROEDITOR_INS_BEFORE 3
#define MACROEDITOR_INS_AFTER  4
#define MACROEDITOR_DELETE     5
static MfMenuItem st_editmenu[] = {
	{ MT_ANCHOR, 0, "Edit data", { 0 } },
	{ MT_ANCHOR, 0, "Change type", { 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_ANCHOR, 0, "Insert before", { 0 } },
	{ MT_ANCHOR, 0, "Insert after", { 0 } },
	{ MT_ANCHOR, 0, "Delete", { 0 } }
};

#define MACROEDITOR_BUTTON_CIRCLE    0
#define MACROEDITOR_BUTTON_CROSS     1
#define MACROEDITOR_BUTTON_SQUARE    2
#define MACROEDITOR_BUTTON_TRIANGLE  3
#define MACROEDITOR_BUTTON_UP        4
#define MACROEDITOR_BUTTON_RIGHT     5
#define MACROEDITOR_BUTTON_DOWN      6
#define MACROEDITOR_BUTTON_LEFT      7
#define MACROEDITOR_BUTTON_LTRIG     8
#define MACROEDITOR_BUTTON_RTRIG     9
#define MACROEDITOR_BUTTON_SELECT   10
#define MACROEDITOR_BUTTON_START    11
static int st_buttons[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static MfMenuItem st_edit_buttons[] = {
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_CIRCLE],   "Circle  ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_CROSS],    "Cross   ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_SQUARE],   "Square  ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_TRIANGLE], "Triangle", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_UP],       "Up      ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_RIGHT],    "Right   ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_DOWN],     "Down    ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_LEFT],     "Left    ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_LTRIG],    "L-trig  ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_RTRIG],    "R-trig  ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_SELECT],   "SELECT  ", { "OFF", "ON", 0 } },
	{ MT_OPTION, &st_buttons[MACROEDITOR_BUTTON_START],    "START   ", { "OFF", "ON", 0 } },
};

#define MACROEDITOR_WAITMS_DIGIT 20
char st_edit_waitms[MACROEDITOR_WAITMS_DIGIT]; /* 9,999,999,999,999,999,999マイクロ秒まで */

#define MACROEDITOR_TYPE_DELAY           0
#define MACROEDITOR_TYPE_BUTTONS_PRESS   1
#define MACROEDITOR_TYPE_BUTTONS_RELEASE 2
#define MACROEDITOR_TYPE_BUTTONS_CHANGE  3
static MfMenuItem st_edit_chtype[] = {
	{ MT_ANCHOR, 0, "Delay", { 0 } },
	{ MT_ANCHOR, 0, "Buttons press", { 0 } },
	{ MT_ANCHOR, 0, "Buttons release", { 0 } },
	{ MT_ANCHOR, 0, "Buttons change", { 0 } }
};

static unsigned int macroeditor_count_macro_lines( MacroData *macro );
static void macroeditor_print_button_symbol( unsigned int buttons, char *command, size_t len );
static unsigned long long strtoull(const char *nptr, char **endptr, int base);

static MfMenuReturnCode macroeditor_menu( SceCtrlLatch *pad_latch, MacroData *macro, unsigned int num );
static MfMenuReturnCode macroeditor_edit_data( SceCtrlLatch *pad_latch, MacroData *macro );
static MfMenuReturnCode macroeditor_ch_type( SceCtrlLatch *pad_latch, MacroData *macro );
static MfMenuReturnCode macroeditor_ins_before( SceCtrlLatch *pad_latch, MacroData *macro );
static MfMenuReturnCode macroeditor_ins_after( SceCtrlLatch *pad_latch, MacroData *macro );
static MfMenuReturnCode macroeditor_delete( SceCtrlLatch *pad_latch, MacroData *macro );


void macroeditorReset( void )
{
	st_macro_all_lines    = 0;
	st_selected_macro_num = 0;
}

MfMenuReturnCode macroeditorMain( SceCtrlLatch *pad_latch, SceCtrlData *pad_data, MacroData *macro )
{
	static bool run_menu = false;
	
	int y = 2, line = 1, rest;
	bool redraw = false;
	
	/* メニュー表示 */
	if( run_menu ){
		switch( macroeditor_menu( pad_latch, macro, st_selected_macro_num ) ){
			case MR_CONTINUE: run_menu = true;  break;
			case MR_BACK:     run_menu = false; break;
			case MR_ENTER:    break; /* MR_ENTERが返ることはない。コンパイラの警告抑止の為。 */
		}
		return MR_CONTINUE;
	}
	
	/* 操作 */
	if( ! st_macro_all_lines ){
		st_macro_all_lines = macroeditor_count_macro_lines( macro );
		if( st_selected_macro_num > st_macro_all_lines ) st_selected_macro_num = st_macro_all_lines;
	}
	
	if( pad_latch->uiMake & PSP_CTRL_CROSS ){
		macroeditorReset();
		return MR_BACK;
	} else if( pad_latch->uiMake & PSP_CTRL_UP && st_selected_macro_num ){
	if( st_selected_macro_num > 14 ) redraw = true;
		st_selected_macro_num--;
	} else if( pad_latch->uiMake & PSP_CTRL_DOWN && st_selected_macro_num < st_macro_all_lines ){
	if( st_selected_macro_num >= 14 ) redraw = true;
		st_selected_macro_num++;
	} else if( pad_latch->uiMake & PSP_CTRL_CIRCLE ){
		/* 0にリセットして、編集後に再取得するよう促す */
		st_macro_all_lines = 0;
		
		run_menu = true;
		return MR_CONTINUE;
	}
	
	/* スクロール */
	for( rest = st_selected_macro_num - 14; rest > 0 && macro; ( macro = macro->next ), line++, rest-- ) continue;
	
	/* 表示開始 */
	for( rest = 28; macro && rest; ( macro = macro->next ), line++, rest-- ){
		char command[255];
		
		if( macro->action == MA_DELAY ){
			sprintf( command, "% 3d: Delay -----------> %lld ms", line, macro->data );
		} else if( macro->action == MA_BUTTONS_PRESS ){
			sprintf( command, "% 3d: Buttons press ---> ", line );
			macroeditor_print_button_symbol( macro->data, command, 255 );
		} else if( macro->action == MA_BUTTONS_RELEASE ){
			sprintf( command, "% 3d: Buttons release -> ", line );
			macroeditor_print_button_symbol( macro->data, command, 255 );
		} else if( macro->action == MA_BUTTONS_CHANGE ){
			sprintf( command, "% 3d: Buttons change --> ", line );
			macroeditor_print_button_symbol( macro->data, command, 255 );
		}
		
		if( redraw ) blitFillBox( 0, blitOffsetLine( y ), 480, BLIT_CHAR_HEIGHT * 2, MENU_BGCOLOR );
		
		if( line - 1 == st_selected_macro_num ){
			blitString( blitOffsetChar( 3 ), blitOffsetLine( y ), command, MENU_FCCOLOR, MENU_BGCOLOR );
		} else{
			blitString( blitOffsetChar( 3 ), blitOffsetLine( y ), command, MENU_FGCOLOR, MENU_BGCOLOR );
		}
		y++;
	}
	
	redraw = false;
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 31 ), "\x80 = MoveUp, \x82 = MoveDown, \x85 = Edit, \x86 = Back, START = Exit", MENU_FGCOLOR, MENU_BGCOLOR );
	
	return MR_CONTINUE;
}

/*-------------------------------------
	Static関数
--------------------------------------*/
static unsigned int macroeditor_count_macro_lines( MacroData *macro )
{
	unsigned int i;
	for( i = 0; macro->next; ( macro = macro->next ), i++ );
	
	return i;
}

static void macroeditor_print_button_symbol( unsigned int buttons, char *command, size_t len )
{
	if( buttons & PSP_CTRL_UP       ) safe_strncat( command, "\x80 ", len );
	if( buttons & PSP_CTRL_RIGHT    ) safe_strncat( command, "\x81 ", len );
	if( buttons & PSP_CTRL_DOWN     ) safe_strncat( command, "\x82 ", len );
	if( buttons & PSP_CTRL_LEFT     ) safe_strncat( command, "\x83 ", len );
	if( buttons & PSP_CTRL_LTRIGGER ) safe_strncat( command, "L ", len );
	if( buttons & PSP_CTRL_RTRIGGER ) safe_strncat( command, "R ", len );
	if( buttons & PSP_CTRL_TRIANGLE ) safe_strncat( command, "\x84 ", len );
	if( buttons & PSP_CTRL_CIRCLE   ) safe_strncat( command, "\x85 ", len );
	if( buttons & PSP_CTRL_CROSS    ) safe_strncat( command, "\x86 ", len );
	if( buttons & PSP_CTRL_SQUARE   ) safe_strncat( command, "\x87 ", len );
	if( buttons & PSP_CTRL_SELECT   ) safe_strncat( command, "SELECT ", len );
	if( buttons & PSP_CTRL_START    ) safe_strncat( command, "START ", len );
}

static MfMenuReturnCode macroeditor_menu( SceCtrlLatch *pad_latch, MacroData *macro, unsigned int num )
{
	MfMenuReturnCode rc = MR_CONTINUE;
	static int selected = -1;
	static MacroeditorEditFunction function = NULL;
	static MacroData *req_macro;
	
	/* selectedが0未満の場合は初回の呼び出しなので描画範囲を塗りつぶし */
	if( selected < 0 ){
		blitFillRect( 150, 40, 290, 120, MENU_BGCOLOR );
		blitLineRect( 150, 40, 290, 120, MENU_FGCOLOR );
		selected = 0;
	}
	
	if( ! function ){
		switch( mfMenuVertical( blitOffsetChar( 25 ), blitOffsetLine( 7 ), BLIT_SCR_WIDTH, st_editmenu, ARRAY_NUM( st_editmenu ), &selected ) ){
			case MR_CONTINUE: break;
			case MR_ENTER:
				switch( selected ){
					case MACROEDITOR_EDIT_DATA:  function = macroeditor_edit_data;  break;
					case MACROEDITOR_CH_TYPE:    function = macroeditor_ch_type;    break;
					case MACROEDITOR_INS_BEFORE: function = macroeditor_ins_before; break;
					case MACROEDITOR_INS_AFTER:  function = macroeditor_ins_after;  break;
					case MACROEDITOR_DELETE:     function = macroeditor_delete;     break;
				}
				for( req_macro = macro; num; num-- ){
					if( req_macro->next ){
						req_macro = req_macro->next;
					} else{
						break;
					}
				}
				break;
			case MR_BACK:
				rc = MR_BACK;
		}
	} else{
		if( ( function )( pad_latch, req_macro ) == MR_BACK ){
				function = NULL;
				rc = MR_BACK;
		}
	}
	
	if( rc == MR_BACK ){
		mfClearColor ( MENU_BGCOLOR );
		selected = -1;
	}
	
	return rc;
}

static MfMenuReturnCode macroeditor_edit_data( SceCtrlLatch *pad_latch, MacroData *macro )
{
	static int selected = -1;
	
	if( macro->action == MA_DELAY ){
		int x = 0;
		int i;
		if( selected < 0 ){
			blitFillBox( 10, 10, 50, 50, MENU_BGCOLOR );
			blitLineBox( 10, 10, 50, 50, MENU_FGCOLOR );
			
			if( macro->data > MACRO_MAX_DELAY ) macro->data = MACRO_MAX_DELAY;
			
			/* 初期値をセットしたいがなぜか落ちる */
			//sprintf( st_edit_waitms, "%020lld", macro->data );
			
			/* 仕方ないので全桁0を初期値に */
			memset( st_edit_waitms, '0', MACROEDITOR_WAITMS_DIGIT );
			
			selected = MACROEDITOR_WAITMS_DIGIT - 1;
		}
		
		if( pad_latch->uiMake & PSP_CTRL_CROSS ){
			selected = -1;
			return MR_BACK;
		} else if( pad_latch->uiMake & PSP_CTRL_CIRCLE ){
			macro->data = strtoull( st_edit_waitms, NULL, 10 );
			selected = -1;
			return MR_BACK;
		} else if( pad_latch->uiMake & PSP_CTRL_UP ){
			if( st_edit_waitms[selected] == '9' ){
				st_edit_waitms[selected] = '0';
			} else{
				st_edit_waitms[selected]++;
			}
		} else if( pad_latch->uiMake & PSP_CTRL_DOWN ){
			if( st_edit_waitms[selected] == '0' ){
				st_edit_waitms[selected] = '9';
			} else{
				st_edit_waitms[selected]--;
			}
		} else if( pad_latch->uiMake & PSP_CTRL_LEFT && selected ){
			selected--;
		} else if( pad_latch->uiMake & PSP_CTRL_RIGHT && selected < MACROEDITOR_WAITMS_DIGIT - 1 ){
			selected++;
		}
		
		for( i = 0; i < MACROEDITOR_WAITMS_DIGIT; i++ ){
			if( i == selected ){
				blitChar( x, 212, '\x80', MENU_FCCOLOR, MENU_BGCOLOR );
				blitChar( x, 220, st_edit_waitms[i], MENU_FCCOLOR, MENU_BGCOLOR );
				blitChar( x, 228, '\x82', MENU_FCCOLOR, MENU_BGCOLOR );
			} else{
				blitChar( x, 212, '\x20', MENU_FGCOLOR, MENU_BGCOLOR );
				blitChar( x, 220, st_edit_waitms[i], MENU_FGCOLOR, MENU_BGCOLOR );
				blitChar( x, 228, '\x20', MENU_FGCOLOR, MENU_BGCOLOR );
			}
			x += BLIT_CHAR_WIDTH;
		}
		
	} else{
		if( selected < 0 ){
			blitFillBox( 120, 30, 120, 115, MENU_BGCOLOR );
			blitLineBox( 120, 30, 120, 115, MENU_FGCOLOR );
			
			st_buttons[MACROEDITOR_BUTTON_CIRCLE]   = macro->data & PSP_CTRL_CIRCLE   ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_CROSS]    = macro->data & PSP_CTRL_CROSS    ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_SQUARE]   = macro->data & PSP_CTRL_SQUARE   ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_TRIANGLE] = macro->data & PSP_CTRL_TRIANGLE ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_UP]       = macro->data & PSP_CTRL_UP       ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_RIGHT]    = macro->data & PSP_CTRL_RIGHT    ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_DOWN]     = macro->data & PSP_CTRL_DOWN     ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_LEFT]     = macro->data & PSP_CTRL_LEFT     ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_LTRIG]    = macro->data & PSP_CTRL_LTRIGGER ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_RTRIG]    = macro->data & PSP_CTRL_RTRIGGER ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_SELECT]   = macro->data & PSP_CTRL_SELECT   ? 1 : 0;
			st_buttons[MACROEDITOR_BUTTON_START]    = macro->data & PSP_CTRL_START    ? 1 : 0;
			
			selected = 0;
		}
		
		switch( mfMenuVertical( 130, 40, 110, st_edit_buttons, ARRAY_NUM( st_edit_buttons ), &selected ) ){
			case MR_ENTER:
			case MR_CONTINUE:
				break;
			case MR_BACK:
				macro->data = 0;
				if( st_buttons[MACROEDITOR_BUTTON_CIRCLE]   ) macro->data |= PSP_CTRL_CIRCLE;
				if( st_buttons[MACROEDITOR_BUTTON_CROSS]    ) macro->data |= PSP_CTRL_CROSS;
				if( st_buttons[MACROEDITOR_BUTTON_SQUARE]   ) macro->data |= PSP_CTRL_SQUARE;
				if( st_buttons[MACROEDITOR_BUTTON_TRIANGLE] ) macro->data |= PSP_CTRL_TRIANGLE;
				if( st_buttons[MACROEDITOR_BUTTON_UP]       ) macro->data |= PSP_CTRL_UP;
				if( st_buttons[MACROEDITOR_BUTTON_RIGHT]    ) macro->data |= PSP_CTRL_RIGHT;
				if( st_buttons[MACROEDITOR_BUTTON_DOWN]     ) macro->data |= PSP_CTRL_DOWN;
				if( st_buttons[MACROEDITOR_BUTTON_LEFT]     ) macro->data |= PSP_CTRL_LEFT;
				if( st_buttons[MACROEDITOR_BUTTON_LTRIG]    ) macro->data |= PSP_CTRL_LTRIGGER;
				if( st_buttons[MACROEDITOR_BUTTON_RTRIG]    ) macro->data |= PSP_CTRL_RTRIGGER;
				if( st_buttons[MACROEDITOR_BUTTON_SELECT]   ) macro->data |= PSP_CTRL_SELECT;
				if( st_buttons[MACROEDITOR_BUTTON_START]    ) macro->data |= PSP_CTRL_START;
				selected = -1;
				return MR_BACK;
		}
	}
	
	return MR_CONTINUE;
}

static MfMenuReturnCode macroeditor_ch_type( SceCtrlLatch *pad_latch, MacroData *macro )
{
	static int selected = -1;
	
	if( selected < 0 ){
		blitFillRect( 150, 40, 290, 110, MENU_BGCOLOR );
		blitLineRect( 150, 40, 290, 110, MENU_FGCOLOR );
		selected = 0;
	}
	
	switch( mfMenuVertical( blitOffsetChar( 24 ), blitOffsetLine( 7 ), BLIT_SCR_WIDTH, st_edit_chtype, ARRAY_NUM( st_edit_chtype ), &selected ) ){
		case MR_CONTINUE: return MR_CONTINUE;
		case MR_ENTER:
			switch( selected ){
				case MACROEDITOR_TYPE_DELAY:
					macro->action = MA_DELAY;
					macro->data   = 0;
					break;
				case MACROEDITOR_TYPE_BUTTONS_PRESS:
					macro->action = MA_BUTTONS_PRESS;
					macro->data   = 0;
					break;
				case MACROEDITOR_TYPE_BUTTONS_RELEASE:
					macro->action = MA_BUTTONS_RELEASE;
					macro->data   = 0;
					break;
				case MACROEDITOR_TYPE_BUTTONS_CHANGE:
					macro->action = MA_BUTTONS_CHANGE;
					macro->data   = 0;
					break;
			}
		case MR_BACK:
			selected = -1;
			break;
	}
	
	return MR_BACK;
}

static MacroData *macroeditor_create_new_command( void )
{
	MacroData *new_command = (MacroData *)MemSceMallocEx( 16, PSP_MEMPART_KERNEL_1, "macroData", PSP_SMEM_Low, sizeof( MacroData ), 0 );
	new_command->action = MA_DELAY;
	new_command->data   = 0;
	new_command->next   = NULL;
	new_command->prev   = NULL;
	
	return new_command;
}

static MfMenuReturnCode macroeditor_ins_before( SceCtrlLatch *pad_latch, MacroData *macro )
{
	MacroData *new_command = macroeditor_create_new_command();
	
	/* 前のコマンドが無い場合は先頭なので特別扱い */
	if( ! macro->prev ){
		/* 新しいデータ域に先頭のデータをコピー */
		*new_command = *macro;
		
		/* 新しいデータ域のprevは先頭を指すように */
		new_command->prev = macro;
		
		/* 先頭コマンドは新規データをnextで指す */
		macro->next = new_command;
		
		/* 先頭データは初期データに変更 */
		macro->action = MA_DELAY;
		macro->data   = 0;
	} else{
		macro->prev->next = new_command;
		new_command->prev = macro->prev;
		macro->prev       = new_command;
		new_command->next = macro;
	}
	
	return MR_BACK;
}

static MfMenuReturnCode macroeditor_ins_after( SceCtrlLatch *pad_latch, MacroData *macro )
{
	MacroData *new_command = macroeditor_create_new_command();
	
	if( macro->next ){
		macro->next->prev = new_command;
		new_command->next = macro->next;
	}
	
	macro->next = new_command;
	new_command->prev = macro;
	
	/* 後ろに追加したのでフォーカスを一つ次へずらす */
	st_selected_macro_num++;
	
	return MR_BACK;
}

static MfMenuReturnCode macroeditor_delete( SceCtrlLatch *pad_latch, MacroData *macro )
{
	/* 前のコマンドが無い場合は先頭なので特別扱い */
	if( ! macro->prev ){
		/* 次のコマンドも無い場合は自分だけなので単に初期値セット */
		if( ! macro->next ){
			macro->action = MA_DELAY;
			macro->data   = 0;
		} else{
			/* 次のコマンドのアドレスを保持 */
			MacroData *rm_macro = macro->next;
			
			/* 次のコマンドの内容を先頭にコピー */
			*macro = *rm_macro;
			
			/* 先頭なので前のコマンドはなしに */
			macro->prev = NULL;
			
			/* 最後に解放するためにアドレスを代入 */
			macro = rm_macro;
		}
	} else{
		macro->prev->next = macro->next;
		if( macro->next ){
			macro->next->prev = macro->prev;
		}
	}
	
	MemSceFree( macro );
	
	return MR_BACK;
}



/*-----------------------------------------------
	strtoullとそれに使う一連の関数。
	USE_KERNEL_LIBCだとlibcがないようで……。
	だからといってこれはダサイと思うんだけど本当はどうすればいいんだ……。
-----------------------------------------------*/
#define ULLONG_MAX 18446744073709551615ULL
int isspace( int c )
{
	if(
		c == ' ' ||
		c == '\f' ||
		c == '\n' ||
		c == '\r' ||
		c == '\t' ||
		c == '\v'
	) return 1;
	return 0;
}
int isupper( int c )
{
	if( c >= 'A' && c <= 'Z' ) return 1;
	return 0;
}
int islower( int c )
{
	if( c >= 'a' && c <= 'z' ) return 1;
	return 0;
}
int isalpha( int c )
{
	if( isupper( c ) || islower( c ) ) return 1;
	return 0;
}
static unsigned long long strtoull(const char *nptr, char **endptr, int base)
{
	/* PSPSDKのstrtoulの型を変更しただけ */
	
	const char *s = nptr;
	unsigned long long acc;
	int c;
	unsigned long long cutoff;
	int any, cutlim;
	
	do
	{
		c = *s++;
	} while (isspace(c));
	
	if (c == '-')
	{
		c = *s++; 	 
	} else if (c == '+')
		c = *s++;
	
	if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X'))
	{
		c = s[1];
		s += 2;
		base = 16;
	}
	
	if (base == 0)
		base = c == '0' ? 8 : 10;
	
	cutoff = ULLONG_MAX;
	cutlim = cutoff % (unsigned long long)base;
	cutoff /= (unsigned long long)base; 	 
	
	for (acc = 0, any = 0;; c = *s++)
	{
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		
		if (c >= base)
			break;
		
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else
		{
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	
	if (any < 0)
	{
		acc = ULLONG_MAX;
		//errno = E_LIB_MATH_RANGE;
		// TODO
		//errno = 30;
	}
	
	if (endptr != 0)
	*endptr = (char *) (any ? s - 1 : nptr);

	return (acc);
}


/*
	macroapi.c
*/

#include "macromgr.h"

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static MacroData    *st_curMacro  = NULL;
static unsigned int st_macroCount = 0;

/*=============================================*/

MacroData *macromgrNew( void )
{
	st_curMacro = (MacroData *)memsceMalloc( sizeof( MacroData ) );
	if( ! st_curMacro ) return NULL;
	
	st_macroCount = 1;
	
	memset( st_curMacro, 0, sizeof( MacroData ) );
	return st_curMacro;
}

MacroData *macromgrInsert( MacroInsertPosition pos, MacroData *macro )
{
	MacroData *addedcmd;
	
	if( st_macroCount >= INT_MAX || ! macro ) return NULL;
	
	addedcmd = (MacroData *)memsceMalloc( sizeof( MacroData ) );
	if( ! addedcmd ) return NULL;
	
	memset( addedcmd, 0, sizeof( MacroData ) );
	
	if( pos == MIP_AFTER ){
		if( macro->next ) macro->next->prev = addedcmd;
		addedcmd->next = macro->next;
		addedcmd->prev = macro;
		macro->next    = addedcmd;
	} else if( pos == MIP_BEFORE ){
		/* 前のコマンドが無い場合は先頭なので特別扱い */
		if( ! macro->prev ){
			/* 新規コマンドに現在の先頭コマンドをコピー */
			*addedcmd = *macro;
			
			/* 新規コマンドの"前"は先頭を指す */
			addedcmd->prev = macro;
			
			/* 先頭は新規コマンドを指す */
			macro->next = addedcmd;
			
			/* この時点で、先頭の次に新規コマンドがリンクしている状態で
			先頭と新規コマンドのコマンド内容は同じ。
			なので、先頭のコマンドを初期化。*/
			macro->action = MA_DELAY;
			macro->data   = 0;
		} else{
			macro->prev->next = addedcmd;
			addedcmd->prev = macro->prev;
			addedcmd->next = macro;
			macro->prev    = addedcmd;
		}
	} else{
		return NULL;
	}
	
	st_macroCount++;
	
	return addedcmd;
}

void macromgrRemove( MacroData *macro )
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
			
			/* 次の次のコマンドがある場合は、それは削除されるコマンドを指しているので自分を指す */
			if( macro->next ) macro->next->prev = macro;
			
			/* メモリ解放 */
			memsceFree( rm_macro );
		}
	} else{
		macro->prev->next = macro->next;
		if( macro->next ){
			macro->next->prev = macro->prev;
		}
		memsceFree( macro );
	}
	
	if( st_macroCount > 1 ) st_macroCount--;
}

void macromgrDestroy( void )
{
	MacroData *macro_data;
	
	if( ! st_curMacro ) return;
	
	if( ! st_curMacro->next ){
		memsceFree( st_curMacro );
	} else{
		for( macro_data = st_curMacro->next; macro_data->next; macro_data = macro_data->next ){
			memsceFree( macro_data->prev );
		}
		memsceFree( macro_data->prev );
		memsceFree( macro_data );
	}
	
	st_curMacro   = NULL;
	st_macroCount = 0;
}

MacroData *macromgrGetFirst( void )
{
	return st_curMacro;
}

int macromgrGetCount( void )
{
	return (signed int)st_macroCount;
}

void macromgrCmdInit( MacroData *macro, MacroAction action, uint64_t data, uint64_t sub )
{
	macro->action = action;
	macro->data   = data;
	macro->sub    = sub;
	
	if( ! data ){
		if( action == MA_ANALOG_MOVE ){
			macro->data = MACROMGR_SET_ANALOG_XY( 128, 128 );
		}
	}
	
	if( ! sub ){
		if( action == MA_RAPIDFIRE_START ){
			macro->sub = MACROMGR_SET_RAPIDDELAY( MACROMGR_DEFAULT_PRESS_DELAY, MACROMGR_DEFAULT_RELEASE_DELAY );
		}
	}
}

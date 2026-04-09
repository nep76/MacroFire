/*
	macroapi.c
*/

#include "macromgr.h"

/*-----------------------------------------------
	スタティック関数
-----------------------------------------------*/

/*-----------------------------------------------
	スタティック変数
-----------------------------------------------*/

/*=============================================*/
MacroData *macromgrNew( void )
{
	MacroData *macro = (MacroData *)memsceMalloc( sizeof( MacroData ) );
	if( ! macro ) return NULL;
	
	macro->action = MA_DELAY;
	macro->data   = 0;
	macro->sub    = 0;
	macro->prev   = NULL;
	macro->next   = NULL;
	
	return macro;
}

MacroData *macromgrInsertBefore( MacroData *macro )
{
	MacroData *newcmd;
	
	if( ! macro ) return NULL;
	
	newcmd = (MacroData *)memsceMalloc( sizeof( MacroData ) );
	if( ! newcmd ) return NULL;
	
	/* 前のコマンドが無い場合は、先頭なので特別扱い */
	if( ! macro->prev ){
		/* 新規コマンドに現在の先頭コマンドをコピー */
		*newcmd = *macro;
		
		/* 新規コマンドの"前"は先頭を指す */
		newcmd->prev = macro;
		
		/* 先頭は新規コマンドを指す */
		macro->next = newcmd;
		
		/* この時点で、先頭の次に新規コマンドがリンクしている状態で
		先頭と新規コマンドのコマンド内容は同じ。
		なので、先頭のコマンドを指定されたコマンドに設定。*/
		macro->action = MA_DELAY;
		macro->data   = 0;
		macro->sub    = 0;
		
		/* 先頭を返す */
		return macro;
	} else{
		macro->prev->next = newcmd;
		newcmd->action    = MA_DELAY;
		newcmd->data      = 0;
		newcmd->sub       = 0;
		newcmd->prev      = macro->prev;
		newcmd->next      = macro;
		macro->prev       = newcmd;
		
		return newcmd;
	}
}

MacroData *macromgrInsertAfter( MacroData *macro )
{
	MacroData *newcmd;
	
	if( ! macro ) return NULL;
	
	newcmd = (MacroData *)memsceMalloc( sizeof( MacroData ) );
	if( ! newcmd ) return NULL;
	
	if( macro->next ) macro->next->prev = newcmd;
	newcmd->action = MA_DELAY;
	newcmd->data   = 0;
	newcmd->sub    = 0;
	newcmd->next   = macro->next;
	newcmd->prev   = macro;
	macro->next    = newcmd;
	
	return newcmd;
}

void macromgrRemove( MacroData *macro )
{
	/* 前のコマンドが無い場合は先頭なので特別扱い */
	if( ! macro->prev ){
		/* 次のコマンドも無い場合は自分だけなので単に初期値セット */
		if( ! macro->next ){
			macro->action = MA_DELAY;
			macro->data   = 0;
			macro->sub    = 0;
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
}


void macromgrDestroy( MacroData *macro )
{
	if( ! macro ) return;
	
	if( ! macro->next ){
		memsceFree( macro );
	} else{
		MacroData *nxm;
		for( nxm = macro->next; nxm->next; nxm = nxm->next ){
			memsceFree( nxm->prev );
		}
		memsceFree( nxm->prev );
		memsceFree( nxm );
	}
}

void macromgrSet( MacroData *macro, MacroAction action, uint64_t data, uint64_t sub )
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

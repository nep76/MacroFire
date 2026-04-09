/*
	malloc_sce.c
*/
#include <pspkernel.h>

#include <string.h>
#include "memsce.h"


#define POWER_OF_TWO( x ) ( ! ( x & ( x - 1 ) ) )

#ifndef ALIGNOFFSET
#define ALIGNOFFSET(x, align) (((x)+((align)-1))&~((align)-1))
#endif

struct heap_header {
	char is_free;
	SceUID	block_id;
	unsigned int size;
};

void *MemSceMallocEx( SceSize boundary, SceUID partitionid, const char *name, int type, SceSize size, void *addr )
{
	struct heap_header hhead;
	void *heap_start;
	
	hhead.block_id = sceKernelAllocPartitionMemory( partitionid, name, type, sizeof( struct heap_header ) + size + boundary, addr );
	if( hhead.block_id < 0 ) return 0;
	
	/* MemSceRealloc()のために確保されたサイズを記録 */
	hhead.size = size;
	
	/* メモリは利用中であるとマーク */
	hhead.is_free = 0;
	
	/* 先頭アドレスを取得 */
	heap_start = sceKernelGetBlockHeadAddr( hhead.block_id );
	heap_start = (void *)ALIGNOFFSET( (unsigned int)heap_start + sizeof( struct heap_header ), boundary );
	
	/* 管理情報分余分に確保しているので、それを書き込む */
	memcpy( (void *)((unsigned int)heap_start - sizeof( struct heap_header )), &hhead, sizeof( struct heap_header ) );
	
	return heap_start;
}

int MemSceFree( void *heap_start )
{
	struct heap_header *hhead;
	if( ! heap_start ) return 0;
	
	/* 指定アドレスから管理情報分だけ戻って管理情報取得 */
	hhead = (struct heap_header *)((unsigned int)heap_start - sizeof( struct heap_header ));
	if( hhead->is_free == 1 ) return -1;
	
	/* メモリが利用可能であるとマークして解放 */
	hhead->is_free = 1;
	
	return sceKernelFreePartitionMemory( hhead->block_id );
}

void *MemSceMalloc( SceSize size )
{
	return MemSceMallocEx( 16, PSP_MEMPART_USER_1, "MemSceMalloc", PSP_SMEM_Low, size, 0 );
}

void *MemSceCalloc( SceSize blk_num, SceSize size )
{
	void *mem = MemSceMalloc( blk_num * size );
	if( ! mem ) return 0;
	
	memset( mem, 0, blk_num * size );
	return mem;
}

void *MemSceRealloc( void *src_heap_start, SceSize resize )
{
	struct heap_header *src_hhead;
	void *new_heap_start;
	
	src_hhead = ( struct heap_header *)((unsigned int)src_heap_start - sizeof( struct heap_header ));
	if( src_hhead->is_free ) return 0;
	
	new_heap_start = MemSceMalloc( resize );
	if( src_hhead->size > resize ){
		memcpy( new_heap_start, src_heap_start, resize );
	} else{
		memcpy( new_heap_start, src_heap_start, src_hhead->size );
	}
	
	MemSceFree( src_heap_start );
	
	return new_heap_start;
}

void *MemSceMemalign( SceSize boundary, SceSize size )
{
	/* バウンダリ境界が2のべき乗かどうか調べる */
	if( boundary <= 0 || ! POWER_OF_TWO( boundary ) ) return 0;
	
	return MemSceMallocEx( boundary, PSP_MEMPART_USER_1, "MemSceMemalign", PSP_SMEM_Low, size, 0 );
}

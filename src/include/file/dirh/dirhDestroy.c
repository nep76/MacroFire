/* dirhDestroy.c */

#include "dirh_types.h"

static void dirh_thread_dopen_exit( struct dirh_thread_dopen_params *thdopen );

void dirhDestroy( DirhUID uid )
{
	if( ! uid ) return;
	
	if( ((struct dirh_params *)uid)->entry.list ) __dirh_clear_entries( &(((struct dirh_params *)uid)->entry) );
	
	if( ((struct dirh_params *)uid)->thdopen ){
		__dirh_wait_for_dopen_thread( ((struct dirh_params *)uid)->thdopen, PSP_THREAD_WAITING );
		dirh_thread_dopen_exit( ((struct dirh_params *)uid)->thdopen );
	}
	
	memoryFree( (void *)uid );
}

static void dirh_thread_dopen_exit( struct dirh_thread_dopen_params *thdopen )
{
	if( thdopen->selfThreadId ){
		thdopen->path = NULL;
		sceKernelWakeupThread( thdopen->selfThreadId );
	}
}

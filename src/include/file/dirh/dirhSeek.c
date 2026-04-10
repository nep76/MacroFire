/* dirhSeek.c */

#include "dirh_types.h"

void dirhSeek( DirhUID uid, DirhWhence whence, int offset )
{
	struct dirh_params *params = (struct dirh_params *)uid;
	
	switch( whence ){
		case DIRH_SEEK_SET: params->entry.pos = 0; break;
		case DIRH_SEEK_END: params->entry.pos = params->entry.count; break;
		case DIRH_SEEK_CUR: break;
	}
	
	params->entry.pos += offset;
	
	if( params->entry.pos < 0 ){
		params->entry.pos = 0;
	} else if( params->entry.pos > params->entry.count ){
		params->entry.pos = params->entry.count;
	}
}

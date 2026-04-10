/* dirhRead.c */

#include "dirh_types.h"

DirhFileInfo *dirhRead( DirhUID uid )
{
	struct dirh_params *params = (struct dirh_params *)uid;
	
	if( params->entry.count <= params->entry.pos ) return NULL;
	
	return &(params->entry.list[params->entry.pos++]);
}

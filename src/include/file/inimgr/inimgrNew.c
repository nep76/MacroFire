/* inimgrNew.c */

#include "inimgr_types.h"

IniUID inimgrNew( void )
{
	struct inimgr_params *params;
	DmemUID dmem = dmemNew( 0, DMEM_HIGH );
	
	if( ! dmem ) return 0;
	
	params = (struct inimgr_params *)dmemAlloc( dmem, sizeof( struct inimgr_params ) );
	if( ! params ){
		dmemDestroy( dmem );
		return 0;
	}
	params->dmem      = dmem;
	params->callbacks = NULL;
	
	/* エントリの有無にかかわらず、デフォルトセクションは作成 */
	params->index = (struct inimgr_section *)dmemAlloc( params->dmem, sizeof( struct inimgr_section ) + strlen( INIMGR_DEFAULT_SECTION_NAME ) + 1 );
	if( ! params->index ){
		dmemDestroy( params->dmem );
		return 0;
	}
	
	params->index->name = (char *)( (uintptr_t)(params->index) + sizeof( struct inimgr_section ) );
	
	strcpy( params->index->name, INIMGR_DEFAULT_SECTION_NAME );
	params->index->entry = NULL;
	params->index->prev  = NULL;
	params->index->next  = NULL;
	params->last  = params->index;
	
	return (IniUID)params;
}

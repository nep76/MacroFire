/* padctrlDestroy.c */

#include "padctrl_types.h"

void padctrlDestroy( PadctrlUID uid )
{
	if( uid ) memoryFree( (void *)uid );
}

/* padctrlSetRepeatButtons.c */

#include "padctrl_types.h"

void padctrlSetRepeatButtons( PadctrlUID uid, PadutilButtons buttons )
{
	if( ! buttons ) return;
	((struct padctrl_params *)uid)->buttons = buttons;
}

/* ovmsgSetPosition.c */

#include "ovmsg_types.h"

void ovmsgSetPosition( OvmsgUID uid, unsigned short x, unsigned short y )
{
	((struct ovmsg_params *)uid)->x = x;
	((struct ovmsg_params *)uid)->y = y;
}

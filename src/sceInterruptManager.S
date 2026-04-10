	.set noreorder

#include "pspimport.s"

	IMPORT_START "InterruptManager",0x00010000
	IMPORT_FUNC  "InterruptManager",0xEEE43F47,	sceKernelRegisterUserSpaceIntrStack

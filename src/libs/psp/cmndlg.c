/*
	cmndlg.c
*/

#include "cmndlg.h"

static SceUID st_semaid = 0;

void cmndlgInit( void )
{
	st_semaid = sceKernelCreateSema( "ClassgCommonDialog", 0, 1, 1, 0 );
}

void cmndlgShutdown( void )
{
	sceKernelDeleteSema( st_semaid );
}

void cmndlgLock( void )
{
	sceKernelWaitSema( st_semaid, 1, 0 );
}

void cmndlgUnlock( void )
{
	sceKernelSignalSema( st_semaid, 1 );
}

bool cmndlgErrorCodeIsSce( int errcode )
{
	if( ! errcode ) return false;
	
	return (unsigned int)errcode < 0xE0000000 ? true : false;
}

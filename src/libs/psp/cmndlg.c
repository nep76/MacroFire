/*
	cmndlg.c
*/

#include "cmndlg.h"

static SceUID st_semaid = 0;
static u32 st_fgcolor = 0xffffffff;
static u32 st_bgcolor = 0xff000000;
static u32 st_fccolor = 0xff0000ff;

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


u32 cmndlgGetFgColor( void )
{
	return st_fgcolor;
}

u32 cmndlgGetBgColor( void )
{
	return st_bgcolor;
}

u32 cmndlgGetFcColor( void )
{
	return st_fccolor;
}

void cmndlgSetFgColor( u32 color )
{
	st_fgcolor = color;
}

void cmndlgSetBgColor( u32 color )
{
	st_bgcolor = color;
}

void cmndlgSetFcColor( u32 color )
{
	st_fccolor = color;
}

/*
	cmndlg.c
*/

#include "cmndlg.h"

static SceUID st_semaid;

static struct cmndlg_ctrl_remap st_ctrlremap;

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

void cmndlgSetAlternativeButtons( CtrlpadAltBtn *altbtn, unsigned int num )
{
	if( ! altbtn ) return;
	
	st_ctrlremap.list = altbtn;
	st_ctrlremap.num  = num;
}

CtrlpadAltBtn *cmndlgGetAlternativeButtonsList( void )
{
	return st_ctrlremap.list;
}

unsigned int cmndlgGetAlternativeButtonsListCount( void )
{
	return st_ctrlremap.num;
}

void cmndlgClearAlternativeButtons( void )
{
	st_ctrlremap.list = NULL;
	st_ctrlremap.num  = 0;
}

bool cmndlgErrorCodeIsSce( int errcode )
{
	if( ! errcode ) return false;
	
	return (unsigned int)errcode < 0xE0000000 ? true : false;
}

/*
	Hook header
*/

#ifndef HOOK_H
#define HOOK_H

#include <pspkernel.h>
#include <psputilsforkernel.h>
#include <pspsdk.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

enum hook_find_module_method {
	HOOK_FIND_MODULE_METHOD_ADDR,
	HOOK_FIND_MODULE_METHOD_UID,
	HOOK_FIND_MODULE_METHOD_NAME
};

void *hookFindSyscallAddr( void *addr );

void *hookUpdateExportedAddr( void **addr, void *entrypoint );
void *hookFindExportedAddr( const char *modname, const char *libname, unsigned int nid );
void *hookGetExportedAddrPointer( SceLibraryEntryTable *libent, unsigned int nid );
SceLibraryEntryTable *hookGetLibraryEntry( SceModule *modinfo, const char *libname );

#ifdef __cplusplus
}
#endif

#endif

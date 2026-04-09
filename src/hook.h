/*
	Hook header
*/

#ifndef __HOOK_H__
#define __HOOK_H__

#include <pspkernel.h>
#include <psputilsforkernel.h>
#include <pspsdk.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "psp/blit.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void *addr;
	void *func;
} SysApi;

unsigned int hookApiByNid( SysApi *sysapi, const char *modname, const char *library, unsigned int nid, void *func );
void *hookRestoreAddr( SysApi *syscall );

#ifdef __cplusplus
}
#endif

#endif

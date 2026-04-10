/* dirh_types.h                   */
/* 内部で使用。外から参照しない。 */

#include "../dirh.h"

#include <stdlib.h>
#include <string.h>
#include "memory/memory.h"
#include "util/makepath.h"
#include "cgerrno.h"

struct dirh_cwd {
	char  *path;
	size_t length;
};

struct dirh_entry {
	DirhFileInfo *list;
	unsigned int count;
	int          pos;
};

struct dirh_thread_dopen_params {
	SceUID     selfThreadId;
	const char *path;
	bool       completed;
	struct dirh_entry *entry;
};

struct dirh_params {
	struct dirh_cwd cwd;
	struct dirh_entry entry;
	struct dirh_thread_dopen_params *thdopen;
	unsigned int options;
};

void __dirh_clear_entries( struct dirh_entry *entry );
int __dirh_parse_dirent( const char *dirpath, struct dirh_entry *entry );
bool __dirh_wait_for_dopen_thread( struct dirh_thread_dopen_params *thdopen, enum PspThreadStatus stat );

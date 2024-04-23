#pragma once

#include "c.h"

/*
 * hook after file read in pg_checksums.
 * the size of buf is BLKSZ, and offset is aligned with BLKSZ. the extension will modify the buffer
 */
typedef void (*pg_checksums_hook_after_read_type)(const char *fname, size_t offset, char *buf);
extern PGDLLIMPORT pg_checksums_hook_after_read_type pg_checksums_hook_after_read;

/*
 * hook before file write in pg_checksums.
 * the size of buf is BLKSZ, and offset is aligned with BLKSZ. the extension will modify the buffer
 */
typedef void (*pg_checksums_hook_before_write_type)(const char *fname, size_t offset, char *buf);
extern PGDLLIMPORT pg_checksums_hook_before_write_type pg_checksums_hook_before_write;

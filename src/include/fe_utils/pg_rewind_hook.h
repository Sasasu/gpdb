#pragma once

#include "c.h"

typedef struct pg_rewind_hook_after_xlog_read_context {
	uint32 timeline;
	uint64 segno;
	uint32 pageOffset;
	int    segSize;
	const char *datadir;
} PgWaldumpHookAfterXlogReadContext;

typedef void (*pg_rewind_hook_after_xlog_read_type) (char *buf, int32 buf_size, PgWaldumpHookAfterXlogReadContext *context);
extern PGDLLIMPORT pg_rewind_hook_after_xlog_read_type pg_rewind_hook_after_xlog_read;

typedef struct pg_rewind_hook_file_io_context {
	int32 dbid;
	int64 start_off;
} PgRewindHookFileIOContext;

typedef void (*pg_rewind_hook_after_file_read_type) (const char *fname, char *buf, int32 buf_size, PgRewindHookFileIOContext *context);
extern PGDLLEXPORT pg_rewind_hook_after_file_read_type pg_rewind_hook_after_file_read;

typedef char* (*pg_rewind_hook_before_file_write_type) (const char *fname, char *buf, int32 buf_size, PgRewindHookFileIOContext *context);
extern PGDLLEXPORT pg_rewind_hook_before_file_write_type pg_rewind_hook_before_file_write;

typedef int (*pg_rewind_hook_decide_file_action_type)(char *path, void *ctx);
extern PGDLLEXPORT pg_rewind_hook_decide_file_action_type pg_rewind_hook_decide_file_action;

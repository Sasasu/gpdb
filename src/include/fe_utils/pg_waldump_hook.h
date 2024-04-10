#pragma once

#include "c.h"

typedef struct pg_waldump_hook_after_read_context {
	uint32 timeline;
	uint64 segno;
	uint32 pageOffset;
	int    segSize;
} PgWaldumpHookAfterReadContext;

typedef void (*pg_waldump_hook_after_read_type) (char *buf, int32 buf_size, PgWaldumpHookAfterReadContext *context);
extern PGDLLIMPORT pg_waldump_hook_after_read_type pg_waldump_hook_after_read;

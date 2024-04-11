/*-------------------------------------------------------------------------
 *
 * basebackup.h
 *	  Exports from replication/basebackup.c.
 *
 * Portions Copyright (c) 2010-2019, PostgreSQL Global Development Group
 *
 * src/include/replication/basebackup.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef _BASEBACKUP_H
#define _BASEBACKUP_H

#include "nodes/replnodes.h"

/*
 * Minimum and maximum values of MAX_RATE option in BASE_BACKUP command.
 */
#define MAX_RATE_LOWER	32
#define MAX_RATE_UPPER	1048576


typedef struct
{
	char	   *oid;
	char	   *path;
	char	   *rpath;			/* relative path within PGDATA, or NULL */
	int64		size;
} tablespaceinfo;

extern void SendBaseBackup(BaseBackupCmd *cmd);

extern int64 sendTablespace(char *path, bool sizeonly);

/*
 * The hook to modify the read buffer in basebackup
 *
 * Arguments:
 *	fname: the file name, relative path to DataDir
 *	blkno: the block number, starts from 0 in the current segment
 *	buf:   the buffer needs to be modified
 *
 * Returns the new buffer after modified or the original buffer if no need to modify
 * The caller does not need to free the new buffer.
 */
typedef char* (*basebackup_file_before_verify_hook_type)(const char *fname, int blkno, char *buf);
extern PGDLLIMPORT basebackup_file_before_verify_hook_type basebackup_file_before_verify_hook;

#endif							/* _BASEBACKUP_H */

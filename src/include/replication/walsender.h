/*-------------------------------------------------------------------------
 *
 * walsender.h
 *	  Exports from replication/walsender.c.
 *
 * Portions Copyright (c) 2010-2019, PostgreSQL Global Development Group
 *
 * src/include/replication/walsender.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef _WALSENDER_H
#define _WALSENDER_H

#include <signal.h>

#include "fmgr.h"
#include "access/xlogdefs.h"

/*
 * What to do with a snapshot in create replication slot command.
 */
typedef enum
{
	CRS_EXPORT_SNAPSHOT,
	CRS_NOEXPORT_SNAPSHOT,
	CRS_USE_SNAPSHOT
} CRSSnapshotAction;

/* global state */
extern bool am_walsender;
extern bool am_cascading_walsender;
extern bool am_db_walsender;
extern bool wake_wal_senders;
extern volatile sig_atomic_t walsender_ready_to_stop;

/* user-settable parameters */
extern int	max_wal_senders;
extern int	wal_sender_timeout;
extern bool log_replication_commands;
extern int	repl_catchup_within_range;

extern void InitWalSender(void);
extern bool exec_replication_command(const char *query_string);
extern void WalSndErrorCleanup(void);
extern void WalSndSignals(void);
extern Size WalSndShmemSize(void);
extern void WalSndShmemInit(void);
extern void WalSndWakeup(void);
extern void WalSndInitStopping(void);
extern void WalSndWaitStopping(void);
extern void HandleWalSndInitStopping(void);
extern void WalSndRqstFileReload(void);
extern XLogRecPtr WalSndCtlGetXLogCleanUpTo(void);
extern void WalSndSetXLogCleanUpTo(XLogRecPtr xlogPtr);

typedef int (*WalSend_hook_type) (struct StringInfoData *buf);
extern PGDLLIMPORT WalSend_hook_type WalSend_hook;

/*
 * Remember that we want to wakeup walsenders later
 *
 * This is separated from doing the actual wakeup because the writeout is done
 * while holding contended locks.
 */
#define WalSndWakeupRequest() \
	do { wake_wal_senders = true; } while (0)

/*
 * wakeup walsenders if there is work to be done
 */
#define WalSndWakeupProcessRequests()		\
	do										\
	{										\
		if (wake_wal_senders)				\
		{									\
			wake_wal_senders = false;		\
			if (max_wal_senders > 0)		\
				WalSndWakeup();				\
		}									\
	} while (0)

#endif							/* _WALSENDER_H */

/*-------------------------------------------------------------------------
 *
 * smgr.h
 *	  storage manager switch public interface declarations.
 *
 *
 * Portions Copyright (c) 2006-2008, Greenplum inc
 * Portions Copyright (c) 2012-Present VMware, Inc. or its affiliates.
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/storage/smgr.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef SMGR_H
#define SMGR_H

#include "lib/ilist.h"
#include "storage/block.h"
#include "storage/relfilenode.h"
#include "storage/dbdirnode.h"

typedef enum SMgrImplementation
{
	SMGR_MD = 0,
	SMGR_AO = 1
} SMgrImpl;

/*
 * smgr.c maintains a table of SMgrRelation objects, which are essentially
 * cached file handles.  An SMgrRelation is created (if not already present)
 * by smgropen(), and destroyed by smgrclose().  Note that neither of these
 * operations imply I/O, they just create or destroy a hashtable entry.
 * (But smgrclose() may release associated resources, such as OS-level file
 * descriptors.)
 *
 * An SMgrRelation may have an "owner", which is just a pointer to it from
 * somewhere else; smgr.c will clear this pointer if the SMgrRelation is
 * closed.  We use this to avoid dangling pointers from relcache to smgr
 * without having to make the smgr explicitly aware of relcache.  There
 * can't be more than one "owner" pointer per SMgrRelation, but that's
 * all we need.
 *
 * SMgrRelations that do not have an "owner" are considered to be transient,
 * and are deleted at end of transaction.
 */
typedef struct SMgrRelationData
{
	/* rnode is the hashtable lookup key, so it must be first! */
	RelFileNodeBackend smgr_rnode;	/* relation physical identifier */

	/* pointer to owning pointer, or NULL if none */
	struct SMgrRelationData **smgr_owner;

	/*
	 * These next three fields are not actually used or manipulated by smgr,
	 * except that they are reset to InvalidBlockNumber upon a cache flush
	 * event (in particular, upon truncation of the relation).  Higher levels
	 * store cached state here so that it will be reset when truncation
	 * happens.  In all three cases, InvalidBlockNumber means "unknown".
	 */
	BlockNumber smgr_targblock; /* current insertion target block */
	BlockNumber smgr_fsm_nblocks;	/* last known size of fsm fork */
	BlockNumber smgr_vm_nblocks;	/* last known size of vm fork */

	/* additional public fields may someday exist here */

	/*
	 * Fields below here are intended to be private to smgr.c and its
	 * submodules.  Do not touch them from elsewhere.
	 */
	SMgrImpl	smgr_which;		/* storage manager selector */

	/*
	 * for md.c; per-fork arrays of the number of open segments
	 * (md_num_open_segs) and the segments themselves (md_seg_fds).
	 */
	int			md_num_open_segs[MAX_FORKNUM + 1];
	struct _MdfdVec *md_seg_fds[MAX_FORKNUM + 1];

	/* if unowned, list link in list of all unowned SMgrRelations */
	dlist_node	node;
} SMgrRelationData;

typedef SMgrRelationData *SMgrRelation;

#define SmgrIsTemp(smgr) \
	RelFileNodeBackendIsTemp((smgr)->smgr_rnode)

extern void smgrinit(void);
extern SMgrRelation smgropen(RelFileNode rnode, BackendId backend,
							 SMgrImpl smgr_which);
extern bool smgrexists(SMgrRelation reln, ForkNumber forknum);
extern void smgrsetowner(SMgrRelation *owner, SMgrRelation reln);
extern void smgrclearowner(SMgrRelation *owner, SMgrRelation reln);
extern void smgrclose(SMgrRelation reln);
extern void smgrcloseall(void);
extern void smgrclosenode(RelFileNodeBackend rnode);
extern void smgrcreate(SMgrRelation reln, ForkNumber forknum, bool isRedo);
extern void smgrcreate_ao(RelFileNodeBackend rnode, int32 segmentFileNum, bool isRedo);
extern void smgrdounlinkall(SMgrRelation *rels, int nrels, bool isRedo);
extern void smgrdounlinkfork(SMgrRelation reln, ForkNumber forknum, bool isRedo);
extern void smgrextend(SMgrRelation reln, ForkNumber forknum,
					   BlockNumber blocknum, char *buffer, bool skipFsync);
extern void smgrprefetch(SMgrRelation reln, ForkNumber forknum,
						 BlockNumber blocknum);
extern void smgrread(SMgrRelation reln, ForkNumber forknum,
					 BlockNumber blocknum, char *buffer);
extern void smgrwrite(SMgrRelation reln, ForkNumber forknum,
					  BlockNumber blocknum, char *buffer, bool skipFsync);
extern void smgrwriteback(SMgrRelation reln, ForkNumber forknum,
						  BlockNumber blocknum, BlockNumber nblocks);
extern BlockNumber smgrnblocks(SMgrRelation reln, ForkNumber forknum);
extern void smgrtruncate(SMgrRelation reln, ForkNumber forknum,
						 BlockNumber nblocks);
extern void smgrimmedsync(SMgrRelation reln, ForkNumber forknum);
extern void AtEOXact_SMgr(void);


/*
 * Hook for plugins to collect statistics from storage functions
 * For example, disk quota extension will use these hooks to
 * detect active tables.
 */
typedef void (*file_create_hook_type)(SMgrRelation reln, ForkNumber forknum, bool isRedo);
extern PGDLLIMPORT file_create_hook_type file_create_hook;

typedef void (*file_create_ao_hook_type)(RelFileNodeBackend rnode, int32 segmentFileNum, bool isRedo);
extern PGDLLIMPORT file_create_ao_hook_type file_create_ao_hook;

typedef void (*file_unlink_hook_type)(SMgrRelation reln, ForkNumber forknum, bool isRedo);
extern PGDLLIMPORT file_unlink_hook_type file_unlink_hook;

typedef char* (*file_extend_hook_type)(SMgrRelation reln, ForkNumber forknum, BlockNumber blocknum, char *buffer, bool skipFsync);
extern PGDLLIMPORT file_extend_hook_type file_extend_hook;

typedef void (*file_writeback_hook_type)(SMgrRelation reln, ForkNumber forknum, BlockNumber blocknum, BlockNumber nblocks);
extern PGDLLIMPORT file_writeback_hook_type file_writeback_hook;

typedef char* (*file_write_hook_type)(SMgrRelation reln, ForkNumber forknum, BlockNumber blocknum, char *buffer, bool skipFsync);
extern PGDLLIMPORT file_write_hook_type file_write_hook;

typedef void (*file_prefetch_hook_type)(SMgrRelation reln, ForkNumber forknum, BlockNumber blocknum);
extern PGDLLIMPORT file_prefetch_hook_type file_prefetch_hook;

typedef void (*file_truncate_hook_type)(SMgrRelation reln, ForkNumber forknum, BlockNumber nblocks);
extern PGDLLIMPORT file_truncate_hook_type file_truncate_hook;

typedef void (*file_read_hook_type)(SMgrRelation reln, ForkNumber forknum, BlockNumber blocknum,  char *buffer);
extern PGDLLIMPORT file_read_hook_type file_read_hook;

typedef void (*file_immedsync_hook_type)(SMgrRelation reln, ForkNumber forknum);
extern PGDLLIMPORT file_immedsync_hook_type file_immedsync_hook;

#endif							/* SMGR_H */

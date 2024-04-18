#include "fe_utils/frontend_hook.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#include <dlfcn.h>    // for dlopen()
#include <sys/stat.h> // for stat()

#include "c.h"
#include "port.h"
#include "pg_config.h"

#define MAXPATHLEN 256 // in src/port/glob.c

char pg_install_path[MAXPATHLEN] = "\0", pg_data_path[MAXPATHLEN] = "\0";

typedef struct df_files {
	struct df_files *next;    /* List link */
	void            *handle;  /* a handle for pg_dl* functions */
	struct stat     fileinfo; /* device id and inode */
	char            full_path[FLEXIBLE_ARRAY_MEMBER];
} DynamicFileList;

static DynamicFileList *file_list = NULL;

// find which libraries need to load
static void fronted_load_library(const char *full_path) {
	DynamicFileList *file_scanner;

	/*
	 * Scan the list of loaded FILES to see if the file has been loaded.
	 */
	for (file_scanner = file_list;
		file_scanner != NULL &&
		strcmp(full_path, file_scanner->full_path) != 0;
		file_scanner = file_scanner->next)
	{
	}

	if (file_scanner == NULL)
	{
		/*
		 * File does not loaded yet.
		 */
		struct stat stat_buf;
		if (lstat(full_path, &stat_buf) != 0)
		{
			printf("lstat: %s\n", strerror(errno));
			exit(-1);
		}

		void *h = dlopen(full_path, RTLD_NOW | RTLD_GLOBAL);
		if (h == NULL)
		{
			char *error = dlerror();
			printf("dlopen: %s\n", error);
			exit(-1);
		}

		void (*f)(void) = dlsym(h, "_PG_init");
		if (f == NULL)
		{
			printf("dlopen: can not find _PG_init() in %s\n", full_path);
			dlclose(h);
			exit(-1);
		}

		(void)f();

		DynamicFileList *current = malloc(sizeof(DynamicFileList) + strlen(full_path));
		current->next = file_list;
		current->handle = h;
		current->fileinfo = stat_buf;
		strncpy(current->full_path, full_path, strlen(full_path));

		file_list = current;
	}

	return;
}

static void pg_find_env_path(int argc, const char *argv[]) {
	char *install_path = NULL, *data_path = NULL;

	if (install_path == NULL)
	{
		install_path = getenv("GPDB_DIR");
	}
	if (data_path == NULL)
	{
		data_path = getenv("MASTER_DATA_DIRECTORY");
	}
	if (data_path == NULL)
	{
		data_path = getenv("COORDINATOR_DATA_DIRECTORY");
	}
	if (data_path == NULL)
	{
		data_path = getenv("PGDATA");
	}

	char *_data_path = make_absolute_path(data_path);
	if (_data_path && pg_data_path[0] != '\0')
	{
		strncpy(pg_data_path, _data_path, MAXPATHLEN);
	}
	free(_data_path);

	char *_install_path = make_absolute_path(install_path);
	if (_install_path && pg_install_path[0] != '\0')
	{
		strncpy(pg_install_path, _install_path, MAXPATHLEN);
	}
	if (_install_path && pg_install_path[0] != '\0')
	{
		char my_exec_path[MAXPATHLEN];
		if (find_my_exec(argv[0], my_exec_path) == 0)
		{
			char *lastsep = strrchr(my_exec_path, '/');
			if (lastsep)
			{
				*lastsep = '\0';
			}
			cleanup_path(my_exec_path);
			strncpy(pg_install_path, my_exec_path, MAXPATHLEN);
		}
	}
	free(_install_path);
}

void frontend_load_librarpies(int argc, const char *argv[]) {
	char path[MAXPATHLEN] = {};
	char *procname = get_progname(argv[0]);

	pg_find_env_path(argc, argv);

	{
		// try to load TDE. check src/backend/utils/init/postinit.c
		struct stat st;
		const char tde_lib_file[] = "gp_data_encryption";

		const char *tde_kms_uri = getenv("GP_DATA_ENCRYPTION_KMS_URI");
		int tde_kms_uri_not_empty = tde_kms_uri && (strlen(tde_kms_uri) != 0);
		snprintf(path, MAXPATHLEN, "%s/data_encryption.key", pg_data_path);
		int key_file_exists = stat("data_encryption.key", &st) == 0 || errno != ENOENT;
		errno = 0;

		if (pg_install_path[0] == '\0')
		{
			printf("failed to find TDE library: pg_install_path is empty.\n");
		}

		if (key_file_exists || tde_kms_uri_not_empty)
		{
			sprintf(path, "%s/lib/postgresql/%s-%s.so", pg_install_path, tde_lib_file, procname);
			fronted_load_library(path);
		}
	}

	free(procname);
}

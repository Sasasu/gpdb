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

char FrontednHookPkglibPath[MAXPGPATH] = {};
char FrontendHookPgDataPath[MAXPGPATH] = {};

typedef struct df_files {
	struct df_files *next;    /* List link */
	void            *handle;  /* a handle for pg_dl* functions */
	struct stat     fileinfo; /* device id and inode */
	char            full_path[FLEXIBLE_ARRAY_MEMBER];
} DynamicFileList;

/* same as server, do not call dlclose for loaded libraries, check internal_unload_library before version 14 */
static DynamicFileList *file_list = NULL;

// find which libraries need to load
void frontend_load_library(const char *path) {
	char *full_path = make_absolute_path(path);

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
		 * File has not been loaded yet.
		 */
		struct stat stat_buf;
		if (lstat(full_path, &stat_buf) != 0)
		{
			printf("lstat: %s %s\n", full_path, strerror(errno));
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

		DynamicFileList *current = malloc(sizeof(DynamicFileList) + strlen(full_path) + 1);
		memset(current, 0, sizeof(DynamicFileList) + strlen(full_path) + 1);
		current->next = file_list;
		current->handle = h;
		current->fileinfo = stat_buf;
		strcpy(current->full_path, full_path);

		file_list = current;
	}

	free(full_path);

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
		data_path = getenv("PGDATA");
	}

	if (data_path && FrontendHookPgDataPath[0] != '\0')
	{
		strncpy(FrontendHookPgDataPath, data_path, MAXPGPATH);
	}

	if (install_path && FrontednHookPkglibPath[0] != '\0')
	{
		strncpy(FrontednHookPkglibPath, install_path, MAXPGPATH);
	}

	if (FrontednHookPkglibPath[0] != '\0' && argc >= 1)
	{
		char my_exec_path[MAXPGPATH] = {};
		if (find_my_exec(argv[0], my_exec_path) == 0)
		{
			/* from <datadir>/bin/<myexec> to <datadir>/bin */
			char *lastsep = strrchr(my_exec_path, '/');
			if (lastsep)
			{
				*lastsep = '\0';
			}

			/* from <datadir>/bin to <datadir> */
			lastsep = strrchr(my_exec_path, '/');
			if (lastsep)
			{
				*lastsep = '\0';
			}

			cleanup_path(my_exec_path);
			strncpy(FrontednHookPkglibPath, my_exec_path, MAXPGPATH);
		}
	}

	if (FrontednHookPkglibPath[0] != '\0')
	{
		char *install_path = make_absolute_path(FrontednHookPkglibPath);
		strncpy(FrontednHookPkglibPath, install_path, MAXPGPATH);
		free(install_path);
	}

	if (FrontendHookPgDataPath[0] != '\0')
	{
		char *data_path = make_absolute_path(FrontendHookPgDataPath);
		strncpy(FrontendHookPgDataPath, data_path, MAXPGPATH);
		free(data_path);
	}
}

void frontend_load_librarpies(int argc, const char *argv[]) {
	char path[MAXPGPATH] = {};
	const char *procname = get_progname(argv[0]);

	pg_find_env_path(argc, argv);

	{
		// try to load TDE. see comment in src/backend/utils/init/postinit.c
		struct stat st;
		const char tde_lib_file[] = "gp_data_encryption";

		const char *tde_kms_uri = getenv("GP_DATA_ENCRYPTION_KMS_URI");
		int tde_kms_uri_not_empty = tde_kms_uri && (strlen(tde_kms_uri) != 0);
		snprintf(path, MAXPGPATH, "%s/data_encryption.key", FrontendHookPgDataPath);
		int key_file_exists = stat("data_encryption.key", &st) == 0 || errno != ENOENT;
		errno = 0;

		if (FrontednHookPkglibPath[0] == '\0')
		{
			printf("failed to find TDE library: FrontednHookPkglibPath is empty.\n");
			exit(-1);
		}

		if (key_file_exists || tde_kms_uri_not_empty)
		{
			sprintf(path, "%s/%s-%s.so", FrontednHookPkglibPath, tde_lib_file, procname);
			frontend_load_library(path);
		}
	}

	free((void*)procname);
}

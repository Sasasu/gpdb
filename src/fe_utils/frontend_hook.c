#include "fe_utils/frontend_hook.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <dlfcn.h>    // for dlopen()
#include <sys/stat.h> // for stat()

#include "c.h"
#include "port.h"

#define MAXPATHLEN 256 // in src/port/glob.c

char pg_install_path[MAXPATHLEN] = "\0", pg_data_path[MAXPATHLEN] = "\0";

static void *dlhandle[128] = {0};
static int dlhandle_size = 0;

// find which libraries need to load
static void fronted_load_library(const char *full_path) {
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

	if (dlhandle_size >= 127)
	{
		printf("too many libraries to load");
		exit(-1);
	}

	for (int i = 0; i < dlhandle_size; ++i)
	{
		if (dlhandle[i] == h)
		{
			return; // dll loaded, pass
		}
	}

	dlhandle[dlhandle_size++] = h;

	void (*f)(void) = dlsym(h, "_PG_init");
	if (f == NULL)
	{
		printf("dlopen: can not find _PG_init() in %s\n", full_path);
		exit(-1);
	}

	f();

	return;
}

static void pg_find_env_path() {
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

	char *_install_path = make_absolute_path(install_path);
	char *_data_path = make_absolute_path(data_path);

	if (_install_path)
	{
		strncpy(pg_install_path, _install_path, MAXPATHLEN);
		free(_install_path);
	}

	if (_data_path)
	{
		strncpy(pg_data_path, _data_path, MAXPATHLEN);
		free(_data_path);
	}
}

void frontend_load_libraries(const char *procname) {
	char path[MAXPATHLEN] = {};

	pg_find_env_path();

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
}

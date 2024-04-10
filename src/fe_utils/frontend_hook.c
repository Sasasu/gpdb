#include "fe_utils/frontend_hook.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dlfcn.h>    // for dlopen()
#include <sys/stat.h> // for stat()

#include "c.h"
#include "port.h"

#define MAXPATHLEN 256                    // in src/port/glob.c
#define CONFIG_FILENAME "postgresql.conf" // in src/backend/utils/misc/guc.c

char *pg_install_path, *pg_data_path;
static int libraries_num;
static char **libraries_path;

static void *dlhandle[128] = {0};
static int dlhandle_size = 0;

int linebufferptr = 0, linebuffersize = 0;
char *linebuffer = NULL;

static char *get_config_file() {
	struct stat stat_buf;

	if (pg_data_path == NULL ||                 // has configdir
		lstat(pg_data_path, &stat_buf) != 0 ||  // configdir exist
		(stat_buf.st_mode & S_IFDIR) != S_IFDIR // configdir is a directory
	) {
		return NULL;
	}

	char *fname = malloc(strlen(pg_data_path) + strlen(CONFIG_FILENAME) + 2);
	sprintf(fname, "%s/%s", pg_data_path, CONFIG_FILENAME);

	return fname;
}

// input K=V, TODO use ParseConfigFp()
// if K == name, return V
static char *sample_parse_config_file(char *K, char *KV) {
	if (KV == NULL)
	{
		return NULL;
	}

	// skip all empty chars
	while (*KV == ' ' || *KV == '\t')
	{
		KV++;
	}

	// start with # or line end
	if (*KV == '\0' || *KV == '#')
	{
		return NULL;
	}

	// check if start by K
	if (strstr(KV, K) != KV)
	{
		return NULL;
	}

	// skip all K character
	while (*K == *KV)
	{
		K++;
		KV++;
	}

	// skip all empty and the '=' char
	while (*KV == ' ' || *KV == '\t' || *KV == '=')
	{
		KV++;
	}

	// trim all after '#'
	int v_len = strlen(KV);
	for (int i = v_len - 1; i >= 0; --i)
	{
		if (KV[i] == '#')
		{
			memset((void*)&KV[i], 0, v_len - i);
			break;
		}
	}

	// trim all empty at the end
	v_len = strlen(KV);
	for (int i = v_len - 1; i >= 0; --i)
	{
		if (KV[i] == ' ' || KV[i] == '\t')
		{
			KV[i] = '\0';
		}
		else
		{
			break;
		}
	}

	// return the content
	return KV;
}

static char *get_shared_preload_lib_internal() {
	char *config_file = get_config_file();

	FILE *f = fopen(config_file, "r");
	if (f == NULL)
	{
		return NULL;
	}

	char ch = 0;
	while ((ch = getc(f)) != EOF)
	{
		if (linebuffersize <= linebufferptr)
		{
			linebuffersize = (linebuffersize + 1) * 2;
			linebuffer = realloc(linebuffer, linebuffersize);
			assert(linebuffer != NULL);
		}

		if (ch == '\n')
		{
			char *kv = sample_parse_config_file("shared_preload_libraries", linebuffer);

			if (kv != NULL)
			{
			  return kv;
			}

			memset(linebuffer, 0, linebufferptr);
			linebufferptr = 0;
			continue;
		}

		linebuffer[linebufferptr] = ch;
		linebufferptr++;
	}

	free(config_file);
	return NULL;
}

// open file + read GUC shared_preload_libraries
static void get_shared_preload_libraries(char ***libs, int *size) {
	char *line = get_shared_preload_lib_internal();
	int line_lenght = strlen(line);

	// NULL or only two '\''
	if (line == NULL || line_lenght <= 2)
	{
		return;
	}

	// example 'foo'
	//             ^ trim this
	if (line[line_lenght - 1] == '\'')
	{
		line[line_lenght - 1] = '\0';
		line_lenght -= 1;
	}

	// example 'foo'
	//         ^ trim this
	if (line[0] == '\'')
	{
		memmove(line, line + 1, line_lenght - 1);
		line[line_lenght-1] = '\0';
		line_lenght -= 1;
	}

	*size = 1;
	for (int i = 0; i < line_lenght; ++i)
	{
		// split by ','
		if (line[i] == ',')
		{
			size++;
		}
	}

	char **ret = malloc(sizeof(int *) * *size);

	char *pt = strtok(line, ",");
	while (pt != NULL)
	{
		int len = strlen(pt);
		ret[*size - 1] = malloc(sizeof(char) * len);
		memset(ret[*size - 1], 0, sizeof(char) * len);
		// trim '.so' if needed
		if (len >= 4 && strcmp(pt + len - 4, ".so") == 0)
		{
			memset(pt + len - 4, '\0', 3);
			len -= 3;
		}
		strncpy(ret[*size - 1], pt, len);

		pt = strtok(NULL, ",");
	}

	*libs = ret;
}

// find which libraries need to load
static void fronted_load_library(const char *full_path) {
	struct stat stat_buf;
	if (lstat(full_path, &stat_buf) != 0)
	{
		return;
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

	// TODO PG_MAGIC_FUNCTION_NAME_STRING

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
	if (pg_install_path == NULL)
	{
		pg_install_path = getenv("GPDB_DIR");
	}
	if (pg_data_path == NULL)
	{
		pg_data_path = getenv("MASTER_DATA_DIRECTORY");
	}
	if (pg_data_path == NULL)
	{
		pg_data_path = getenv("COORDINATOR_DATA_DIRECTORY");
	}

	// memory leak here, but that is OK
	pg_install_path = make_absolute_path(pg_install_path);
	pg_data_path = make_absolute_path(pg_data_path);
}

void frontend_load_libraries(const char *procname) {
	pg_find_env_path();

	for (int i = 0; i < libraries_num; ++i)
	{
		fronted_load_library(libraries_path[i]);
	}

	char *configfile_path = get_config_file();
	if (configfile_path == NULL)
	{
		return;
	}

	int num;
	char **libs = NULL;
	get_shared_preload_libraries(&libs, &num);
	if (libs == NULL)
	{
		return;
	}

	for (int i = 0; i < num; ++i)
	{
		char path[MAXPATHLEN] = {};
		sprintf(path, "%s/lib/postgresql/%s-%s.so", pg_install_path, libs[i], procname);
		fronted_load_library(path);
	}
}

extern void add_library_to_load(const char *lib) {
	char **libs = malloc(sizeof(char *) * libraries_num + 1);
	memcpy(libs, libraries_path, libraries_num);

	libs[libraries_num] = malloc(sizeof(char) * strlen(lib));
	strcpy(libs[libraries_num], lib);

	free(libraries_path);
	libraries_path = libs;
	libraries_num++;
}

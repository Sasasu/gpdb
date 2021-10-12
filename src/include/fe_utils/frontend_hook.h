#pragma once

#ifndef FRONTEND
#define FRONTEND
#endif


extern char *pg_install_path;
extern char *pg_data_path;

extern void add_library_to_load(const char *lib);

// load the fronted libraries
extern void frontend_load_libraries(const char *procname);

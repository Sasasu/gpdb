#pragma once

#ifndef FRONTEND
#define FRONTEND
#endif


extern char *pg_install_path;
extern char *pg_data_source_path;
extern char *pg_data_dest_path;

extern void add_library_to_load(const char *lib);

// load the fronted libraries
extern void frontend_load_libraries(const char *procname);

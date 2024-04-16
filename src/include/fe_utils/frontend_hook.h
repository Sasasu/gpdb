#pragma once

#ifndef FRONTEND
#define FRONTEND
#endif

extern char pg_install_path[], pg_data_path[];

void frontend_load_libraries(const char *procname);

#pragma once

#ifndef FRONTEND
#define FRONTEND
#endif

#include "c.h" // for PGDLLEXPORT and MAXPGPATH

extern PGDLLEXPORT char FrontednHookPkglibPath[MAXPGPATH];
extern PGDLLEXPORT char FrontendHookPgDataPath[MAXPGPATH];

void frontend_load_libraries(int argc, const char *argv[]);
void frontend_load_library(const char *path);

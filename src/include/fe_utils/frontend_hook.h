#pragma once

#ifndef FRONTEND
#define FRONTEND
#endif

#include "c.h" // for PGDLLEXPORT and MAXPGPATH

extern PGDLLEXPORT char FrontednHookPgInstallPath[MAXPGPATH];
extern PGDLLEXPORT char FrontendHookPgDataPath[MAXPGPATH];

void frontend_load_librarpies(int argc, const char *argv[]);
void frontend_load_library(const char *path);

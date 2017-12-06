#ifndef MEMREADER_MODULE_H
#define MEMREADER_MODULE_H

#include "memreader.h"
#include "process.h"

#define MODULE_T MEMREADER_METATABLE(module)

typedef struct {
	HMODULE handle;
	TCHAR name[MAX_PATH];
	TCHAR path[MAX_PATH];
} module_t;

#endif
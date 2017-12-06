#ifndef MEMREADER_MODULE_H
#define MEMREADER_MODULE_H

#include "memreader.h"

#define MODULE_T MEMREADER_METATABLE(module)

typedef struct {
	HMODULE handle;
	TCHAR name[MAX_PATH];
	TCHAR path[MAX_PATH];
} module_t;

void init_module(module_t * module, HMODULE handle, HANDLE process);
int register_module(lua_State *L);
module_t* push_module(lua_State *L);

#endif
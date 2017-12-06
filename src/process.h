#ifndef MEMREADER_PROCESS_H
#define MEMREADER_PROCESS_H

#include "memreader.h"

#define PROCESS_T MT_PREFIX(process)
typedef struct {
	HANDLE handle;
	HANDLE mainModule;
	DWORD pid;
	LPVOID baseAddress;
	TCHAR name[MAX_PATH];
} process_t;

process_t* check_process(lua_State *L, int index);
process_t* push_process(lua_State *L);
void init_process(process_t* process);

int register_process(lua_State *L);

#endif
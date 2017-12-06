#include "module.h"

#include <psapi.h>

void init_module(module_t * module, process_t * process)
{
	GetModuleBaseName(process->handle, module->handle, module->name, sizeof(module->name) / sizeof(TCHAR));
	GetModuleFileNameEx(process->handle, module->handle, module->path, sizeof(module->path) / sizeof(TCHAR));
}

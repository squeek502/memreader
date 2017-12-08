#include "process.h"
#include "address.h"
#include "wutils.h"
#include "module.h"

#include <psapi.h>

process_t* check_process(lua_State *L, int index)
{
	process_t* proc = (process_t*)luaL_checkudata(L, index, PROCESS_T);
	return proc;
}

process_t* push_process(lua_State *L)
{
	process_t *proc = (process_t*)lua_newuserdata(L, sizeof(process_t));
	luaL_getmetatable(L, PROCESS_T);
	lua_setmetatable(L, -2);
	return proc;
}

void init_process(process_t* process, DWORD pid, HANDLE handle)
{
	process->pid = pid;
	process->handle = handle;

	DWORD cb;
	EnumProcessModules(process->handle, &process->module, sizeof(HMODULE), &cb);

	GetModuleBaseName(process->handle, NULL, process->name, sizeof(process->name) / sizeof(TCHAR));
	GetModuleFileNameEx(process->handle, NULL, process->path, sizeof(process->path) / sizeof(TCHAR));
}

static int process_read(lua_State *L)
{
	process_t* process = check_process(L, 1);
	LPVOID address = (LPVOID)memaddress_checkptr(L, 2);
	SIZE_T bytes = luaL_checkint(L, 3);

	char *buff = malloc(bytes);
	SIZE_T numBytesRead;

	if (!ReadProcessMemory(process->handle, address, buff, bytes, &numBytesRead))
		return push_last_error(L);

	lua_pushlstring(L, buff, numBytesRead);
	return 1;
}

static int process_read_relative(lua_State *L)
{
	process_t* process = check_process(L, 1);
	LPVOID address = process->module;
	LONG_PTR offset = memaddress_checkptr(L, 2);
	SIZE_T bytes = luaL_checkint(L, 3);
	address = (LPVOID)((char*)address + offset);
	lua_settop(L, 1); // pop everything but the process_t
	memaddress_t* absAddress = push_memaddress(L);
	absAddress->ptr = address;
	lua_pushinteger(L, bytes);
	return process_read(L);
}

static int process_version(lua_State *L)
{
	process_t* process = check_process(L, 1);

	const char* exe = process->path;

	DWORD len = GetFileVersionInfoSize(exe, NULL);
	if (len == 0)
		return push_last_error(L);

	BYTE* pVersionResource = malloc(len);
	if (!GetFileVersionInfo(exe, 0, len, pVersionResource))
	{
		free(pVersionResource);
		return push_last_error(L);
	}

	UINT uLen;
	VS_FIXEDFILEINFO* ptFixedFileInfo;
	if (!VerQueryValue(pVersionResource, "\\", (LPVOID*)&ptFixedFileInfo, &uLen))
	{
		free(pVersionResource);
		return push_error(L, "unable to get version");
	}

	if (uLen == 0)
	{
		free(pVersionResource);
		return push_error(L, "unable to get version");
	}

	lua_createtable(L, 0, 2);
	lua_createtable(L, 0, 4);
	lua_pushinteger(L, HIWORD(ptFixedFileInfo->dwFileVersionMS));
	lua_setfield(L, -2, "major");
	lua_pushinteger(L, LOWORD(ptFixedFileInfo->dwFileVersionMS));
	lua_setfield(L, -2, "minor");
	lua_pushinteger(L, HIWORD(ptFixedFileInfo->dwFileVersionLS));
	lua_setfield(L, -2, "build");
	lua_pushinteger(L, LOWORD(ptFixedFileInfo->dwFileVersionLS));
	lua_setfield(L, -2, "revision");
	lua_setfield(L, -2, "file");

	lua_createtable(L, 0, 4);
	lua_pushinteger(L, HIWORD(ptFixedFileInfo->dwProductVersionMS));
	lua_setfield(L, -2, "major");
	lua_pushinteger(L, LOWORD(ptFixedFileInfo->dwProductVersionMS));
	lua_setfield(L, -2, "minor");
	lua_pushinteger(L, HIWORD(ptFixedFileInfo->dwProductVersionLS));
	lua_setfield(L, -2, "build");
	lua_pushinteger(L, LOWORD(ptFixedFileInfo->dwProductVersionLS));
	lua_setfield(L, -2, "revision");
	lua_setfield(L, -2, "product");

	free(pVersionResource);

	return 1;
}

static int process_modules(lua_State *L)
{
	process_t* process = check_process(L, 1);
	HMODULE modules[MAX_MODULES];
	DWORD bytesRequired;

	if (!EnumProcessModulesEx(process->handle, modules, sizeof(modules), &bytesRequired, LIST_MODULES_ALL))
		return push_last_error(L);

	UINT numModules = bytesRequired / sizeof(HMODULE);

	lua_createtable(L, numModules, 0);
	for (UINT i = 0; i < numModules; i++)
	{
		module_t* module = push_module(L);
		init_module(module, modules[i], process->handle);
		lua_rawseti(L, -2, i+1);
	}
	return 1;
}

static int process_exit_code(lua_State *L)
{
	process_t* process = check_process(L, 1);
	DWORD exitCode;
	if (!GetExitCodeProcess(process->handle, &exitCode))
		return push_last_error(L);

	if (exitCode == STILL_ACTIVE)
		return 0;

	lua_pushinteger(L, exitCode);
	return 1;
}

static int process_gc(lua_State *L)
{
	process_t* process = check_process(L, 1);
	CloseHandle(process->handle);
	return 0;
}

static const luaL_reg process_meta[] = {
	{ "__gc", process_gc },
	{ NULL, NULL }
};
static const luaL_reg process_methods[] = {
	{ "version", process_version },
	{ "read", process_read },
	{ "read_relative", process_read_relative },
	{ "modules", process_modules },
	{ "exit_code", process_exit_code },
	{ NULL, NULL }
};
static udata_field_info process_getters[] = {
	{ "pid", udata_field_get_int, offsetof(process_t, pid) },
	{ "name", udata_field_get_string, offsetof(process_t, name) },
	{ "path", udata_field_get_string, offsetof(process_t, path) },
	{ "base", udata_field_get_memaddress, offsetof(process_t, module) },
	{ NULL, NULL }
};
static udata_field_info process_setters[] = {
	{ NULL, NULL }
};

int register_process(lua_State *L)
{
	// store methods in a table outside the metatable
	luaL_newlib(L, process_methods);
	int methods = lua_gettop(L);

	// only put the metamethods in the metatable itself
	luaL_newmetatable(L, PROCESS_T);
	luaL_setfuncs(L, process_meta, 0);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methods);
	lua_rawset(L, metatable); // metatable.__metatable = methods

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, metatable);
	register_udata_fields(L, process_getters); // put getters in the metatable
	lua_pushvalue(L, methods);
	// push udata_field_index_handler with the metatable and methods as upvalues
	lua_pushcclosure(L, udata_field_index_handler, 2);
	lua_rawset(L, metatable); // metatable.__index = udata_field_index_handler(metatable, methods)

	lua_pushliteral(L, "__newindex");
	lua_newtable(L); // table for setters
	register_udata_fields(L, process_setters);
	// push udata_field_newindex_handler with the setters table as an upvalue
	lua_pushcclosure(L, udata_field_newindex_handler, 1);
	lua_rawset(L, metatable); // metatable.__newindex = udata_field_newindex_handler(setters)

	lua_pop(L, 2); // pop metatable and methods table
	return 0;
}
#include "process.h"
#include "address.h"
#include "wutils.h"

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

void init_process(process_t* process)
{
	HMODULE module;
	DWORD cb;
	EnumProcessModules(process->handle, &module, sizeof(module), &cb);
	process->mainModule = module;

	GetProcessName(process->pid, process->name, sizeof(process->name) / sizeof(TCHAR));

	MODULEINFO info;
	GetModuleInformation(process->handle, module, &info, sizeof(info));
	process->baseAddress = info.lpBaseOfDll;
}

static int process_get_name(lua_State *L)
{
	process_t* process = check_process(L, 1);
	lua_settop(L, 0);
	lua_pushinteger(L, process->pid);
	return lua_GetProcessName(L);
}

static int process_base_address(lua_State *L)
{
	process_t* process = check_process(L, 1);

	HMODULE module;
	DWORD cb;
	MODULEINFO info;

	if (EnumProcessModules(process->handle, &module, sizeof(module), &cb)
		&& GetModuleInformation(process->handle, module, &info, sizeof(info)))
	{
		LPVOID baseAddress = info.lpBaseOfDll;
		memaddress_t* addr = push_memaddress(L);
		addr->ptr = baseAddress;
		return 1;
	}

	return push_last_error(L);
}


static int process_version(lua_State *L)
{
	process_t* process = check_process(L, 1);

	CHAR gameExe[MAX_PATH];
	GetModuleFileNameEx(process->handle, process->mainModule, gameExe, MAX_PATH);

	DWORD len = GetFileVersionInfoSize(gameExe, NULL);
	if (len == 0)
		return push_last_error(L);

	BYTE* pVersionResource = malloc(len);
	if (!GetFileVersionInfo(gameExe, 0, len, pVersionResource))
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
	{ "get_name", process_get_name },
	{ "base_address", process_base_address },
	{ "version", process_version },
	{ NULL, NULL }
};
static udata_field_info process_getters[] = {
	{ "pid", udata_field_get_int, offsetof(process_t, pid) },
	{ "name", udata_field_get_string, offsetof(process_t, name) },
	{ "baseAddress", udata_field_get_memaddress, offsetof(process_t, baseAddress) },
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
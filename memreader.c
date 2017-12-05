#define LUA_LIB
#define WINVER 0x501
#define _WIN32_WINNT 0x501
#define PSAPI_VERSION 1
#include "lauxlib.h"
#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#include <assert.h>

#define OPEN_PROCESS_FLAGS PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ
#define MAX_PROCESSES 1024

#if LUA_VERSION_NUM < 502
#ifndef luaL_newlib
# define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#endif
# define luaL_setfuncs(L,l,n) (assert(n==0), luaL_register(L,NULL,l))
#endif

/************************************************/
// Utils

int push_error(lua_State *L, const char* msg)
{
	lua_pushnil(L);
	lua_pushstring(L, msg);
	return 2;
}

int push_last_error(lua_State *L)
{
	static char err[256];
	int strLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		GetSystemDefaultLangID(), // Default language
		err, 256, NULL
	);
	err[strLen - 2] = '\0'; // strip the \r\n
	return push_error(L, err);
}

const char *stack_tostring(lua_State *L, int index)
{
	const char *s;
	lua_getglobal(L, "tostring");
	lua_pushvalue(L, index);
	lua_call(L, 1, 1);
	s = lua_tostring(L, -1);
	lua_pop(L, 1);
	return s;
}

BOOL GetProcessName(DWORD processId, TCHAR* processName, DWORD len)
{
	HANDLE hProcess = OpenProcess(OPEN_PROCESS_FLAGS, FALSE, processId);

	if (!hProcess)
		return FALSE;

	BOOL success = GetModuleBaseName(hProcess, NULL, processName, len);
	CloseHandle(hProcess);
	return success;
}

static int get_int(lua_State *L, void *v)
{
	lua_pushnumber(L, *(int*)v);
	return 1;
}

static int set_int(lua_State *L, void *v)
{
	*(int*)v = luaL_checkint(L, 3);
	return 0;
}

typedef int(*udata_field_fn) (lua_State *L, void *v);

/* member info for get and set handlers */
typedef const struct {
	const char *name;  /* member name */
	udata_field_fn func;     /* get or set function for type of member */
	size_t offset;     /* offset of member within your_t */
}  udata_field_info;

typedef udata_field_info* udata_field_reg;

static void register_udata_fields(lua_State *L, udata_field_reg l)
{
	for (; l->name; l++) {
		lua_pushstring(L, l->name);
		lua_pushlightuserdata(L, (void*)l);
		lua_settable(L, -3);
	}
}

static int udata_field_proxy(lua_State *L)
{
	/* for get: stack has userdata, index, lightuserdata */
	/* for set: stack has userdata, index, value, lightuserdata */
	udata_field_info* m = (udata_field_info*)lua_touserdata(L, -1);  /* member info */
	lua_pop(L, 1);                               /* drop lightuserdata */
	luaL_checktype(L, 1, LUA_TUSERDATA);
	return m->func(L, (void *)((char *)lua_touserdata(L, 1) + m->offset));
}

static int index_handler(lua_State *L)
{
	/* stack has userdata, index */
	lua_pushvalue(L, 2);                     /* dup index */
	lua_rawget(L, lua_upvalueindex(1));      /* lookup member by name */
	if (!lua_islightuserdata(L, -1)) {
		lua_pop(L, 1);                         /* drop value */
		lua_pushvalue(L, 2);                   /* dup index */
		lua_gettable(L, lua_upvalueindex(2));  /* else try methods */
		return 1; // return either the method or nil
	}
	return udata_field_proxy(L);                      /* call get function */
}

static int newindex_handler(lua_State *L)
{
	/* stack has userdata, index, value */
	lua_pushvalue(L, 2);                     /* dup index */
	lua_rawget(L, lua_upvalueindex(1));      /* lookup member by name */
	if (!lua_islightuserdata(L, -1))         /* invalid member */
	{
		luaL_error(L, "attempt to set field '%s' of %s", lua_tostring(L, 2), stack_tostring(L, 1));
	}
	return udata_field_proxy(L);                      /* call set function */
}

/************************************************/
// Structs

#define MT_PREFIX(t) "memreader." #t

// Memory Address

#define MT_MEMORY_ADDRESS MT_PREFIX(memaddress)
typedef struct {
	LPVOID ptr;
} memaddress_t;

static memaddress_t* check_memaddress(lua_State *L, int index)
{
	memaddress_t* addr = (memaddress_t*)luaL_checkudata(L, index, MT_MEMORY_ADDRESS);
	return addr;
}

static memaddress_t* push_memaddress(lua_State *L)
{
	memaddress_t *addr = (memaddress_t*)lua_newuserdata(L, sizeof(memaddress_t));
	luaL_getmetatable(L, MT_MEMORY_ADDRESS);
	lua_setmetatable(L, -2);
	return addr;
}

static int memaddress_tostring(lua_State *L)
{
	memaddress_t* addr = check_memaddress(L, 1);
	lua_pushfstring(L, "%p", addr->ptr);
	return 1;
}

static const luaL_reg memaddress_meta[] = {
	{ "__tostring", memaddress_tostring },
	{ NULL, NULL }
};

static int register_memaddress(lua_State *L) {
	luaL_newmetatable(L, MT_MEMORY_ADDRESS);
	luaL_setfuncs(L, memaddress_meta, 0);
	lua_pop(L, 1);
	return 0;
}

// Process

#define MT_PROCESS MT_PREFIX(process)
typedef struct {
	HANDLE handle;
	DWORD pid;
} process_t;

static process_t* check_process(lua_State *L, int index)
{
	process_t* proc = (process_t*)luaL_checkudata(L, index, MT_PROCESS);
	return proc;
}

static process_t* push_process(lua_State *L)
{
	process_t *proc = (process_t*)lua_newuserdata(L, sizeof(process_t));
	luaL_getmetatable(L, MT_PROCESS);
	lua_setmetatable(L, -2);
	return proc;
}

static int lua_GetProcessName(lua_State *L); // forward decl
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
	{ NULL, NULL }
};
static const udata_field_info process_getters[] = {
	{ "pid", get_int, offsetof(process_t, pid) },
	{ NULL, NULL }
};
static const udata_field_info process_setters[] = {
	{ NULL, NULL }
};

static int register_process(lua_State *L) {
	// store methods in a table outside the metatable
	luaL_newlib(L, process_methods);
	int methods = lua_gettop(L);

	// only put the metamethods in the metatable itself
	luaL_newmetatable(L, MT_PROCESS);
	luaL_setfuncs(L, process_meta, 0);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methods);
	lua_rawset(L, metatable); // metatable.__metatable = methods

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, metatable);
	register_udata_fields(L, process_getters); // put getters in the metatable
	lua_pushvalue(L, methods);
	// push index_handler with the metatable and methods as upvalues
	lua_pushcclosure(L, index_handler, 2);
	lua_rawset(L, metatable); // metatable.__index = index_handler(metatable, methods)

	lua_pushliteral(L, "__newindex");
	lua_newtable(L); // table for setters
	register_udata_fields(L, process_setters);
	// push newindex_handler with the setters table as an upvalue
	lua_pushcclosure(L, newindex_handler, 1);
	lua_rawset(L, metatable); // metatable.__newindex = newindex_handler(setters)
	
	lua_pop(L, 2); // pop metatable and methods table
	return 0;
}

/************************************************/
// Module Methods

static int lua_GetProcessName(lua_State *L)
{
	DWORD processId = luaL_checkint(L, 1);
	TCHAR processName[MAX_PATH];
	if (!GetProcessName(processId, processName, MAX_PATH))
		return push_last_error(L);
	lua_pushstring(L, processName);
	return 1;
}

static int lua_SetDebugPrivilege(lua_State *L)
{
	BOOL state = lua_toboolean(L, 1);
	HANDLE hToken = NULL;
	LUID luid;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
		return push_last_error(L);
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
		return push_last_error(L);

	TOKEN_PRIVILEGES tokenPriv;
	tokenPriv.PrivilegeCount = 1;
	tokenPriv.Privileges[0].Luid = luid;
	tokenPriv.Privileges[0].Attributes = state ? SE_PRIVILEGE_ENABLED : 0;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tokenPriv, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
		return push_last_error(L);

	lua_pushboolean(L, TRUE);
	return 1;
}

static int lua_GetProcesses(lua_State *L)
{
	DWORD processes[MAX_PROCESSES]; DWORD bytesFilled;

	if (EnumProcesses(processes, sizeof(processes), &bytesFilled) == 0)
		return push_last_error(L);

	unsigned int numProcesses = bytesFilled / sizeof(DWORD);

	lua_createtable(L, numProcesses-1, 0);
	int k = 1;
	for (unsigned int i = 0; i < numProcesses; i++)
	{
		if (processes[i] != 0)
		{
			lua_pushinteger(L, processes[i]);
			lua_rawseti(L, -2, k++);
		}
	}
	return 1;
}

static int lua_OpenProcess(lua_State *L)
{
	DWORD processId = luaL_checkint(L, 1);

	if (processId <= 0)
		return push_error(L, "invalid process id");

	HANDLE processHandle = OpenProcess(OPEN_PROCESS_FLAGS, 0, processId);

	if (!processHandle)
		return push_last_error(L);

	process_t *process = push_process(L);
	process->handle = processHandle;
	process->pid = processId;

	return 1;
}

static const luaL_Reg memreader_funcs[] = {
	{"open_process", lua_OpenProcess},
	{"debug_privilege", lua_SetDebugPrivilege},
	{"process_name", lua_GetProcessName},
	{"process_ids", lua_GetProcesses},
	{NULL,NULL}
};

LUALIB_API int luaopen_memreader(lua_State *L)
{
	luaL_newlib(L, memreader_funcs);

	register_process(L);
	register_memaddress(L);

	return 1;
}

#include "memreader.h"
#include "process.h"
#include "address.h"
#include "wutils.h"

#include <psapi.h>

int memreader_process_name(lua_State *L)
{
	DWORD processId = luaL_checkint(L, 1);
	TCHAR processName[MAX_PATH];
	if (!GetProcessName(processId, processName, MAX_PATH))
		return push_last_error(L);
	lua_pushstring(L, processName);
	return 1;
}

static int memreader_debug_privilege(lua_State *L)
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

static int memreader_process_ids(lua_State *L)
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

static int memreader_open_process(lua_State *L)
{
	DWORD processId = luaL_checkint(L, 1);

	if (processId <= 0)
		return push_error(L, "invalid process id");

	HANDLE processHandle = OpenProcess(OPEN_PROCESS_FLAGS, 0, processId);

	if (!processHandle)
		return push_last_error(L);

	process_t *process = push_process(L);
	init_process(process, processId, processHandle);

	return 1;
}

static const luaL_Reg memreader_funcs[] = {
	{ "open_process", memreader_open_process },
	{ "debug_privilege", memreader_debug_privilege },
	{ "process_name", memreader_process_name },
	{ "process_ids", memreader_process_ids },
	{ NULL, NULL }
};

LUALIB_API int luaopen_memreader(lua_State *L)
{
	luaL_newlib(L, memreader_funcs);

	register_process(L);
	register_memaddress(L);

	return 1;
}

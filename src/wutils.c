#include "wutils.h"
#include <psapi.h>

BOOL GetProcessName(DWORD processId, TCHAR* processName, DWORD len)
{
	HANDLE hProcess = OpenProcess(OPEN_PROCESS_FLAGS, FALSE, processId);

	if (!hProcess)
		return FALSE;

	BOOL success = GetModuleBaseName(hProcess, NULL, processName, len);
	CloseHandle(hProcess);
	return success;
}
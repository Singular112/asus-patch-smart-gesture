#include "stdafx.h"
#include "Injector.h"


// function pointer declaration.
typedef NTSTATUS (NTAPI *_NtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);


/* minimum buf_size = 29 */
DWORD GetLastErrorDescription(DWORD _In_ last_error, LPTSTR _Out_ error_description, size_t _In_ buf_size)
{
	LPVOID lpMsgBuf = NULL;

	DWORD format_result = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, last_error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

	if (format_result == 0)
	{
		if (lpMsgBuf)
			LocalFree(lpMsgBuf);

		return GetLastError();
	}

	size_t full_msg_size = (29 + format_result) * sizeof(TCHAR);
	if (buf_size < full_msg_size)
	{
		LocalFree(lpMsgBuf);
		return ERROR_INSUFFICIENT_BUFFER;
	}

	StringCchPrintf((LPTSTR)error_description, buf_size, TEXT("Error code: %d (0x%0.8x). Description: %s"), last_error, last_error, lpMsgBuf);

	LocalFree(lpMsgBuf);
	return ERROR_SUCCESS;
}


std::locale russian_locale("Russian");


std::string HexedLastError()
{
	DWORD last_err = GetLastError();
	char buf[256] = "";
	sprintf_s(buf, sizeof(buf), "0x%0.8x", last_err);
	return buf;
}

namespace Injector
{

string_t m_lastError;

bool IsModulePresent(DWORD pid, LPCTSTR moduleName, bool& out_presented)
{
	out_presented = false;

	//
	HANDLE hSnapShot;
	MODULEENTRY32 moduleEntry = { 0 };

	if ((hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid)) == INVALID_HANDLE_VALUE)
	{
		_TCHAR err_descr[1024];
		GetLastErrorDescription(::GetLastError(), err_descr, sizeof(err_descr));

		m_lastError = __T("Failed to call CreateToolhelp32Snapshot with error: ");
		m_lastError += err_descr;
		return false;
	}

	moduleEntry.dwSize = sizeof(MODULEENTRY32);
	BOOL first = Module32First(hSnapShot, &moduleEntry);
	if (first == FALSE)
	{
		_TCHAR err_descr[1024];
		GetLastErrorDescription(::GetLastError(), err_descr, sizeof(err_descr));

		m_lastError = __T("Failed to snap modules-info for process with error: ");
		m_lastError += err_descr;
		return false;
	}

	do
	{
		if (lstrcmpi(moduleEntry.szExePath, moduleName) == 0)
		{
			out_presented = true;
			break;
		}
	}
	while (Module32Next(hSnapShot, &moduleEntry));

	CloseHandle(hSnapShot);

	return true;
}


bool GetProcID(LPCTSTR proc_name, LPCTSTR parent_proc_name, DWORD& out_id)
{
	HANDLE hSnapShot;
	PROCESSENTRY32 processEntry = { 0 };

	out_id = 0;

	if ((hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) == INVALID_HANDLE_VALUE)
	{
		_TCHAR err_descr[1024];
		GetLastErrorDescription(::GetLastError(), err_descr, sizeof(err_descr));

		m_lastError = __T("Failed to call CreateToolhelp32Snapshot with error: ");
		m_lastError += err_descr;
		return false;
	}

	processEntry.dwSize = sizeof(PROCESSENTRY32);
	BOOL first = Process32First(hSnapShot, &processEntry);
	if (first == FALSE)
	{
		_TCHAR err_descr[1024];
		GetLastErrorDescription(::GetLastError(), err_descr, sizeof(err_descr));

		m_lastError = __T("Failed to snap modules-info for process with error: ");
		m_lastError += err_descr;
		return false;
	}

	do
	{
		if (lstrcmpi(processEntry.szExeFile, proc_name) == 0) {
			out_id = processEntry.th32ProcessID;
			break;
		}
	}
	while (Process32Next(hSnapShot, &processEntry));

	CloseHandle(hSnapShot);

	return true;
}


bool setPrivilege(HANDLE hToken, LPCTSTR szPrivName, bool fEnable)
{
	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	LookupPrivilegeValue(NULL, szPrivName, &tp.Privileges[0].Luid);
	tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
	AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
	return ::GetLastError() == ERROR_SUCCESS;
}


bool InjectDLL(LPCTSTR dll_path, LPCTSTR proc_name, LPCTSTR parent_proc_name)
{
	if (dll_path == NULL || proc_name == NULL)
	{
		m_lastError = __T("dll_path or proc_name is null");
		return false;
	}

	DWORD proc_id = 0;
	if (!GetProcID(proc_name, parent_proc_name, proc_id))
	{
		//m_lastError = __T("Failed to GetProcID with error: ") + HexedLastError();
		return false;
	}

	if (proc_id == 0)
	{
		m_lastError = __T("Process not found");
		return false;
	}

	bool already_presented = false;
	if (!IsModulePresent(proc_id, dll_path, already_presented))
	{
		//m_lastError = "IsModulePresent failed with error: " + HexedLastError();
		return false;
	}

	if (already_presented)
	{
		m_lastError = __T("dll already presented");
		return true;
	}

	HANDLE hToken = 0;
	HANDLE hCurrentProc = GetCurrentProcess();
	if (!OpenProcessToken(hCurrentProc, TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		//m_lastError = "Failed to OpenProcessToken with error: " + HexedLastError();
		return false;
	}
	else
	{
		if (!setPrivilege(hToken, SE_DEBUG_NAME, TRUE))
		{
			_TCHAR err_descr[1024];
			GetLastErrorDescription(::GetLastError(), err_descr, sizeof(err_descr));

			m_lastError = __T("setPrivilege failed with error: ");
			m_lastError += err_descr;
			return false;
		}
	}

	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, proc_id);
	if (processHandle == NULL)
	{
		//m_lastError = "Failed to OpenProcess with error: " + HexedLastError();
		return false;
	}

	LPVOID lpProc = (LPVOID)GetProcAddress(GetModuleHandle(__T("kernel32.dll")),
#ifdef _UNICODE
		"LoadLibraryW"
#else
		"LoadLibraryA"
#endif
		);
	if (lpProc == NULL)
	{
		CloseHandle(processHandle);
		//m_lastError = "Failed to GetProcAddress(\"kernel32.dll\") with error: " + HexedLastError();
		return false;
	}

	LPVOID lpParams = VirtualAllocEx(processHandle, NULL, (_tcslen(dll_path) + 1) * sizeof(TCHAR), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (lpParams == NULL)
	{
		CloseHandle(processHandle);
		//m_lastError = "Failed to VirtualAllocEx with error: " + HexedLastError();
		return false;
	}

	SIZE_T dwWritten;
	if (WriteProcessMemory(processHandle, (LPVOID)lpParams, dll_path, (_tcslen(dll_path) + 1) * sizeof(TCHAR), &dwWritten) == 0)
	{
		CloseHandle(processHandle);
		//m_lastError = "Failed to WriteProcessMemory with error: " + HexedLastError();
		return false;
	}

	//HANDLE CreateRemoteThread(
	//	HANDLE hProcess,                         // дескриптор процесса
	//	LPSECURITY_ATTRIBUTES lpThreadAttributes,// дескриптор защиты (SD)
	//	SIZE_T dwStackSize,                      // размер начального стека
	//	LPTHREAD_START_ROUTINE lpStartAddress,   // функция потока
	//	LPVOID lpParameter,                      // аргументы потока
	//	DWORD dwCreationFlags,                   // параметры создания
	//	LPDWORD lpThreadId                       // идентификатор потока
	//	);
	HANDLE thread = CreateRemoteThread(processHandle, 0, 0, (LPTHREAD_START_ROUTINE)lpProc, (LPVOID)lpParams, 0, 0);
	if (thread == NULL)
	{
		CloseHandle(processHandle);
		//m_lastError = "Failed to CreateRemoteThread with error: " + HexedLastError();
		return false;
	}

	return true;
}


string_t GetLastError() {
	return m_lastError;
}

}

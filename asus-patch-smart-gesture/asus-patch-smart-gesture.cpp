// asus-patch-smart-gesture.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "asus-patch-smart-gesture.h"

uint64_t g_hookSwitchTPStatusNoBroadcastAddr = 0;
volatile bool g_running = true;

extern "C"
{
	uint64_t g_hookSwitchTPStatusNoBroadcastRetAddr = 0;

	void asm_hookSwitchTPStatusNoBroadcast();
	void asm_SwitchTPStatusNoBroadcast();

	void SwitchTPStatusNoBroadcastFake(int param);
}

// This is an example of an exported variable
ASUSPATCHSMARTGESTURE_API int nasuspatchsmartgesture=0;

// This is an example of an exported function.
ASUSPATCHSMARTGESTURE_API int fnasuspatchsmartgesture(void)
{
	return 42;
}


#pragma pack(1)
struct patch_t
{
	BYTE nPatchType;
	DWORD dwAddress;
};
#pragma pack()

BOOL ApplyPatch(BYTE eType, uint64_t dwAddress, const void* pTarget)
{
	DWORD dwOldValue, dwTemp;
	patch_t pWrite =
	{
		eType,
		(DWORD)pTarget - (dwAddress + sizeof(DWORD) + sizeof(BYTE))
	};

	VirtualProtect((LPVOID)dwAddress, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &dwOldValue);
	BOOL bSuccess = WriteProcessMemory(GetCurrentProcess(), (LPVOID)dwAddress, &pWrite, sizeof(pWrite), NULL);
	VirtualProtect((LPVOID)dwAddress, sizeof(DWORD), dwOldValue, &dwTemp);
	return bSuccess;
}


typedef void (*SwitchTPStatusNoBroadcast_t)(int32_t);
SwitchTPStatusNoBroadcast_t SwitchTPStatusNoBroadcast = 0;

void __cdecl SwitchTPStatusNoBroadcastFake(int32_t param)
{
	OutputDebugStringA("[asus-patch-smart-gesture] SwitchTPStatusNoBroadcastFake1\n");
	if (SwitchTPStatusNoBroadcast)
	{
		OutputDebugStringA("[asus-patch-smart-gesture] SwitchTPStatusNoBroadcastFake2\n");
		SwitchTPStatusNoBroadcast(param);
	}
}


bool get_prev_status()
{
	std::string file_path = getenv("LOCALAPPDATA");
	file_path += "\\temp";
	file_path += "\\asus-patch-smart-gesture.flag";

	std::ifstream infile;
	infile.open(file_path);
	if (infile)
	{
		char data[100] = "";
		infile >> data;

		if (strcmp(data, "enable") == 0)
		{
			return true;
		}
	}

	return false;
}


void set_prev_status(bool status)
{
	std::string file_path = getenv("LOCALAPPDATA");
	file_path += "\\temp";
	file_path += "\\asus-patch-smart-gesture.flag";

	std::ofstream outfile;
	outfile.open(file_path);
	if (outfile)
	{
		char data[100] = "";
		outfile << (const char*)(status ? "enable" : "disable");
	}
}


void main_thread(LPVOID lpParam)
{
	OutputDebugStringA("[asus-patch-smart-gesture] main_thread entry\n");

	while (g_running)
	{
		static bool prev_status = get_prev_status();
		bool status = check_for_event_occured(); 

		if (prev_status != status)
		{
			SwitchTPStatusNoBroadcastFake(status);
			OutputDebugStringA("[asus-patch-smart-gesture] SwitchTPStatusNoBroadcast\n");
			prev_status = status;
		}
		Sleep(500);
	}
}


bool initialize()
{
	g_hookSwitchTPStatusNoBroadcastAddr = (uint64_t)GetProcAddress
	(
		GetModuleHandle(L"AsusTPApi.dll"),
		"SwitchTPStatusNoBroadcast"
	);

	if (g_hookSwitchTPStatusNoBroadcastAddr == 0)
	{
		OutputDebugStringA("[asus-patch-smart-gesture] Failed to get SwitchTPStatusNoBroadcast address\n");
		return false;
	}

	SwitchTPStatusNoBroadcast
		= (SwitchTPStatusNoBroadcast_t)g_hookSwitchTPStatusNoBroadcastAddr;

	g_hookSwitchTPStatusNoBroadcastRetAddr = g_hookSwitchTPStatusNoBroadcastAddr + 5;

	char buf[4096] = "";
	sprintf_s(buf, "[asus-patch-smart-gesture] TPStatusNoBroadcast address = %ull\n",
		g_hookSwitchTPStatusNoBroadcastAddr);
	OutputDebugStringA(buf);

	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&main_thread, NULL, 0, NULL);

#if 0

	BOOL succ = ApplyPatch(0xE9,
		g_hookSwitchTPStatusNoBroadcastAddr,
		&asm_hookSwitchTPStatusNoBroadcast);

	sprintf_s(buf, "apply patch status: %s (%d)\n",
		succ ? "ok" : "failed",
		GetLastError());
	OutputDebugStringA(buf);
#endif

	return true;
}

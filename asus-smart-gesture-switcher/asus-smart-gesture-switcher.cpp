// asus-smart-gesture-switcher.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "ServiceRunFlagGlobalEvent.h"
HANDLE g_hServiceRunFlagGlobalEvent;

extern "C"
{
	char c_ext_byte = 1;
	unsigned short c_ext_word = 2;
	long c_ext_dword = 3;
	__int64 c_ext_qword = 4;
	void *c_ext_ptr = (void *)(5);
	void c_ext_my_function();

	void hello_world_asm();
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


void test_test()
{
	volatile int bbb = 3434;
}


void c_ext_my_function()
{

}
void __stdcall SwitchTPStatusNoBroadcastFunc(int32_t param)
{
	printf("param = %d\n", param);
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

//int main(int, char**)
int WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd
	)
{
	std::string err;
	SECURITY_DESCRIPTOR sd;
	if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) == FALSE
		|| SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE) == FALSE)
	{
		err.resize(1024);
		sprintf_s(&err[0],
			err.size(),
			"ERROR: [%s] Ошибка инициализации дескриптора безопасности",
			"asus-patch-smart-gesture");
		return false;
	}

	if (!initialize_global_service_status_flag("asus-patch-smart-gesture", err, true))
	{
		OutputDebugStringA(err.c_str());
		return -1;
	}

	bool status = get_prev_status();
	status = !status;
	set_prev_status(status);
	if (status)
	{
		set_global_service_status_flag();
	}
	else
	{
		reset_global_service_status_flag();
	}

	return 0;
}

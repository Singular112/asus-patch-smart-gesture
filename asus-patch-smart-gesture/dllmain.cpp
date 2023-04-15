// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "asus-patch-smart-gesture.h"

HANDLE g_hServiceRunFlagGlobalEvent;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		OutputDebugStringA("[asus-patch-smart-gesture] process attach\n");
		std::string err;
		if (!initialize_global_service_status_flag("asus-patch-smart-gesture", err))
		{
			OutputDebugStringA(err.c_str());
			return -1;
		}
		initialize();
	}
	break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
	{
		OutputDebugStringA("[asus-patch-smart-gesture] process detach\n");

		//g_running = false;
	}
	break;
	}
	return TRUE;
}


#pragma once

//static const char g_szServiceRunFlagEventName[MAX_PATH] = "SFP22";
const wchar_t g_szServiceRunFlagEventName[MAX_PATH] = L"Global\\STFlag";

extern HANDLE g_hServiceRunFlagGlobalEvent;

inline bool initialize_global_service_status_flag(const char* app_name, std::string& err, bool is_slave_process = false)
{
#if 0
	SECURITY_DESCRIPTOR sd;
	if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) == FALSE
		|| SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE) == FALSE)
	{
		err.resize(1024);
		sprintf_s(&err[0], err.size(), "ERROR: [%s] Ошибка инициализации дескриптора безопасности", app_name);
		return false;
	}
#endif

	SECURITY_ATTRIBUTES sa;
	{
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = FALSE;
		sa.lpSecurityDescriptor = /*&sd*/NULL;
	}

	if (is_slave_process)
	{
		g_hServiceRunFlagGlobalEvent = OpenEventW(SYNCHRONIZE, FALSE, g_szServiceRunFlagEventName);
	}
	else
	{
		g_hServiceRunFlagGlobalEvent = CreateEventW(&sa, TRUE, FALSE, g_szServiceRunFlagEventName);
	}

	if (g_hServiceRunFlagGlobalEvent == NULL
		|| g_hServiceRunFlagGlobalEvent == INVALID_HANDLE_VALUE)
	{
		err.resize(1024);
		sprintf_s(&err[0], err.size(), "ERROR: [%s] Ошибка создания глобального флага состояния сервиса, код ошибки %Xh\n", app_name, GetLastError());
		return false;
	}

	if (!is_slave_process)
	{
		if (ResetEvent(g_hServiceRunFlagGlobalEvent) == FALSE)
		{
			err.resize(1024);
			sprintf_s(&err[0], err.size(), "ERROR: [%s] Ошибка сброса глобального флага состояния сервиса, код ошибки %Xh\n", app_name, GetLastError());
			return false;
		}
	}

	return true;
}


static bool set_global_service_status_flag()
{
	if (g_hServiceRunFlagGlobalEvent == NULL
		|| g_hServiceRunFlagGlobalEvent == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	return SetEvent(g_hServiceRunFlagGlobalEvent) != 0;
}


static bool reset_global_service_status_flag()
{
	if (g_hServiceRunFlagGlobalEvent == NULL
		|| g_hServiceRunFlagGlobalEvent == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	return ResetEvent(g_hServiceRunFlagGlobalEvent) != 0;
}


static bool check_for_event_occured()
{
	return g_hServiceRunFlagGlobalEvent != NULL
		&& g_hServiceRunFlagGlobalEvent != INVALID_HANDLE_VALUE
		&& WaitForSingleObject(g_hServiceRunFlagGlobalEvent, 0) == WAIT_OBJECT_0;
}


static bool wait_for_event()
{
	return g_hServiceRunFlagGlobalEvent != NULL
		&& g_hServiceRunFlagGlobalEvent != INVALID_HANDLE_VALUE
		&& WaitForSingleObject(g_hServiceRunFlagGlobalEvent, INFINITE) == WAIT_OBJECT_0;
}

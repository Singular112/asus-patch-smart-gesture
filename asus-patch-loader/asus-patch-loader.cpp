// asus-patch-loader.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "resource.h"

#include "Injector.h"


#define WM_NOTIFYICONMSG (WM_USER + 2)

std::atomic_bool g_need_to_scan_process;

std::atomic_bool g_running;

HINSTANCE g_hInst;
NOTIFYICONDATA tnd;
HMENU hMenu;
HMENU hMenuTrackPopup;


VOID CALLBACK WaitOrTimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	g_need_to_scan_process = true;
	printf("process terminated\n");
}


void HandlePopupMenu(HWND hWnd, POINT point)
{
	LPCTSTR pszIDMenu;

	pszIDMenu = MAKEINTRESOURCE(IDR_MENU2);
	hMenu = LoadMenu(g_hInst, pszIDMenu);
	if (!hMenu)
		return;

	hMenuTrackPopup = GetSubMenu(hMenu, 0);
	TrackPopupMenu(hMenuTrackPopup, 0, point.x, point.y, 0, hWnd, NULL);
	DestroyMenu(hMenu);
}


void AddStatusIcon(HWND hWnd, DWORD dwMessage)
{
	HICON hStatusIcon;
	LPCTSTR pszIDStatusIcon;

	pszIDStatusIcon = MAKEINTRESOURCE(IDI_ICON1);

	hStatusIcon = LoadIcon(g_hInst, pszIDStatusIcon);
	tnd.cbSize = sizeof(NOTIFYICONDATA);
	tnd.hWnd = hWnd;
	tnd.uID = 1;
	tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	tnd.uCallbackMessage = WM_NOTIFYICONMSG;
	tnd.hIcon = hStatusIcon;
	lstrcpyn(tnd.szTip, __T("Asus SG patch loader"), sizeof(tnd.szTip));
	Shell_NotifyIcon(dwMessage, &tnd);
}


void DeleteStatusIcon()
{
	Shell_NotifyIcon(NIM_DELETE, &tnd);
}


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	switch (msg)
	{
	case WM_COMMAND:
	{
		if (wParam == 40001)
		{
			DeleteStatusIcon();
			PostMessage(hWnd, WM_QUIT, 0, 0);
		}
	}
	break;

	case WM_NOTIFYICONMSG:
		switch (lParam)
		{
		case WM_RBUTTONDOWN:
			GetCursorPos(&pt);
			HandlePopupMenu(hWnd, pt);
			break;

		default:
			break;
		}
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	g_hInst = hInst;

	MSG msg;
	HWND hWnd;

	WNDCLASSEX wndclass;
	{
		memset(&wndclass, 0, sizeof(WNDCLASSEX));

		wndclass.style = 0;
		wndclass.cbSize = sizeof(WNDCLASSEX);
		wndclass.lpfnWndProc = (WNDPROC)WndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInst;
		wndclass.hIcon = NULL;
		wndclass.hCursor = NULL;
		wndclass.hbrBackground = NULL;
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = __T("TRAYWNDCLAS");
	}

	if (!RegisterClassEx(&wndclass))
		return FALSE;

	hWnd = CreateWindow(__T("TRAYWNDCLAS"),
		__T(""),
		0,
		0,
		0,
		1,
		1,
		HWND_DESKTOP,
		NULL,
		hInst,
		NULL);

	AddStatusIcon(hWnd, NIM_ADD);

	std::thread t1([&]
	{
		g_running = true;
		g_need_to_scan_process = true;

		while (g_running)
		{
			if (g_need_to_scan_process)
			{
				DWORD procId = 0;
				Injector::GetProcID(__T("AsusTPCenter.exe"),
					__T(""),
					procId);

				if (procId > 0)
				{
					wchar_t buffer[MAX_PATH];
					GetModuleFileName(NULL, buffer, MAX_PATH);
					auto pos = std::wstring(buffer).find_last_of(L"\\/");
					auto dllpath = std::wstring(buffer).substr(0, pos) + L"\\asus-patch-smart-gesture.dll";

					bool is_presented = false;
					Injector::IsModulePresent(procId,
						dllpath.c_str(),
						is_presented);

					bool injected = is_presented;
					if (!is_presented)
					{
						injected = Injector::InjectDLL(dllpath.c_str(), __T("AsusTPCenter.exe"));
					}

					if (injected)
					{
						printf("%s\n", is_presented? "already injected" : "injected");

						HANDLE hProcHandle = OpenProcess(PROCESS_ALL_ACCESS,
							FALSE,
							procId);

						HANDLE hNewHandle;
						RegisterWaitForSingleObject(&hNewHandle,
							hProcHandle,
							WaitOrTimerCallback,
							NULL,
							INFINITE,
							WT_EXECUTEONLYONCE);
						g_need_to_scan_process = false;
					}
					else
					{
						wprintf(__T("failed to inject with error: %s\n"),
							Injector::GetLastError().c_str());
					}
				}
			}

			Sleep(500);
		}
	});

	while (true)
	{
		WaitMessage();

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				g_running = false;
				t1.join();
				break;
			}

			TranslateMessage(&msg); // to process WM_CHAR
			DispatchMessage(&msg);
		}
	}

	return 0;
}


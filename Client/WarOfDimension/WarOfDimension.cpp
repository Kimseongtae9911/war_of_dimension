// WarOfDimension.cpp : 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "WarOfDimension.h"
#include "GameFramework.h"

#include "NetworkManager.h"
#include "OverlapEx.h"
#include "MeshSharingTests.h"
#include "ClientMemoryProfile.h"
#include <locale>

int RunParticleBufferPoolTests(const wchar_t* reportPath);

#define MAX_LOADSTRING 100

HINSTANCE						ghAppInstance;
TCHAR							szTitle[MAX_LOADSTRING];
TCHAR							szWindowClass[MAX_LOADSTRING];

CGameFramework					gGameFramework;

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void SwitchToBorderedMode(HWND hWnd);
void SwitchToBorderlessMode(HWND hWnd);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	int argumentCount = 0;
	auto arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    if (arguments && argumentCount >= 2 && wcscmp(arguments[1], L"--test-particle-buffer-pool") == 0)
    {
        if (argumentCount != 3) { LocalFree(arguments); return 2; }
        const std::wstring report = arguments[2]; LocalFree(arguments);
        return RunParticleBufferPoolTests(report.c_str());
    }
	if (arguments && argumentCount >= 2 && wcscmp(arguments[1], L"--test-particle-selection") == 0)
	{
		if (argumentCount != 3) { LocalFree(arguments); return 2; }
		const std::wstring report = arguments[2]; LocalFree(arguments);
		return RunParticleSelectionTests(report.c_str());
	}
    if (arguments && argumentCount >= 2 && wcscmp(arguments[1], L"--profile-particle-reuse") == 0)
    {
        if (argumentCount != 4 || (wcscmp(arguments[3], L"dedicated") != 0 && wcscmp(arguments[3], L"pooled") != 0))
        { LocalFree(arguments); return 2; }
        const std::wstring report = arguments[2];
        const bool dedicated = wcscmp(arguments[3], L"dedicated") == 0; LocalFree(arguments);
        return RunClientMemoryProfile(gGameFramework, report.c_str(), false, true, false, true, false, true, dedicated);
    }
	if (arguments && argumentCount >= 2 && wcscmp(arguments[1], L"--profile-particle-memory") == 0)
	{
		if (argumentCount != 4 || (wcscmp(arguments[3], L"full") != 0 && wcscmp(arguments[3], L"selected") != 0))
		{ LocalFree(arguments); return 2; }
		const std::wstring report = arguments[2];
		const bool full = wcscmp(arguments[3], L"full") == 0; LocalFree(arguments);
		return RunClientMemoryProfile(gGameFramework, report.c_str(), false, true, false, true, full);
	}
	if (arguments && argumentCount >= 2 && wcscmp(arguments[1], L"--test-hero-selection") == 0)
	{
		if (argumentCount != 3) { LocalFree(arguments); return 2; }
		const std::wstring report = arguments[2];
		LocalFree(arguments);
		return RunHeroSelectionAudit(gGameFramework, report.c_str());
	}
	if (arguments && argumentCount >= 2 && wcscmp(arguments[1], L"--profile-hero-memory") == 0)
	{
		if (argumentCount != 4 || (wcscmp(arguments[3], L"full") != 0 && wcscmp(arguments[3], L"selected") != 0))
		{ LocalFree(arguments); return 2; }
		const std::wstring report = arguments[2];
		const bool full = wcscmp(arguments[3], L"full") == 0;
		LocalFree(arguments);
		return RunClientMemoryProfile(gGameFramework, report.c_str(), false, true, full);
	}
	if (arguments && argumentCount >= 2 && wcscmp(arguments[1], L"--profile-client-memory") == 0)
	{
		if (argumentCount != 4 || (wcscmp(arguments[3], L"legacy") != 0 && wcscmp(arguments[3], L"shared") != 0))
		{ LocalFree(arguments); return 2; }
		const std::wstring report = arguments[2];
		const bool legacy = wcscmp(arguments[3], L"legacy") == 0;
		LocalFree(arguments);
		return RunClientMemoryProfile(gGameFramework, report.c_str(), legacy);
	}
	if (arguments && argumentCount >= 2 &&
		(wcscmp(arguments[1], L"--test-mesh-sharing") == 0 || wcscmp(arguments[1], L"--audit-mesh-assets") == 0))
	{
		if (argumentCount != 3) { LocalFree(arguments); return 2; }
		const bool audit = wcscmp(arguments[1], L"--audit-mesh-assets") == 0;
		const std::wstring report = arguments[2];
		LocalFree(arguments);
		return audit ? AuditMeshAssets(report.c_str()) : RunMeshSharingTests(report.c_str());
	}
	if (arguments) LocalFree(arguments);

	MSG msg;
	HACCEL hAccelTable;

	::LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	::LoadString(hInstance, IDC_WAROFDIMENSION, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

#ifdef Test
	wcout.imbue(std::locale("korean"));
	::AllocConsole();
	FILE* consoleStream;
	::freopen_s(&consoleStream, "CONOUT$", "w", stdout);
	::freopen_s(&consoleStream, "CONIN$", "r", stdin);
#endif
	
	string ip;

#ifndef LOCAL_TEST
	cout << "Input Lobby Server IP: ";
	cin >> ip;
#else
	ip = "127.0.0.1";
#endif // !LOCAL_TEST
	//Connect To Server
	NetworkManager::GetInstance()->Initialize(ip);
	std::thread th{ []() {NetworkManager::GetInstance()->WorkerThread(); } };

	if (!InitInstance(hInstance, nCmdShow)) return(FALSE);	

	hAccelTable = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WAROFDIMENSION));

	while (1)
	{
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) break;
			if (!::TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
		else
		{
			gGameFramework.FrameAdvance();
		}
	}
	gGameFramework.OnDestroy();	
	cout << "Game Destroy" << endl;
	NetworkManager::GetInstance()->Disconnect();

	th.detach();
	cout << "Thread join" << endl;
	
#ifdef Test
	FreeConsole();
#endif Test

	return((int)msg.wParam);
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LABPROJECT0797ANIMATION));
	//wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wcex.hCursor = ::LoadCursorFromFile(L"Cursor/Busy.cur");
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;//MAKEINTRESOURCE(IDC_WAROFDIMENSION);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = ::LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return ::RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	ghAppInstance = hInstance;

	RECT rc = { 0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT };
	DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_BORDER;
	AdjustWindowRect(&rc, dwStyle, FALSE);
	HWND hMainWnd = CreateWindow(szWindowClass, szTitle, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);

	//DWORD dwStyle = WS_POPUP; // Use WS_POPUP style instead of WS_OVERLAPPED
	//HWND hMainWnd = CreateWindow(szWindowClass, szTitle, dwStyle, 0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, NULL, NULL, hInstance, NULL);


	if (!hMainWnd) return(FALSE);

	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = FRAME_BUFFER_WIDTH;
	rect.bottom = FRAME_BUFFER_HEIGHT;
	AdjustWindowRect(&rect, dwStyle, FALSE);

	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	int x = (screenWidth - width) / 2;
	int y = (screenHeight - height) / 2;

	SetWindowPos(hMainWnd, NULL, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);


	gGameFramework.OnCreate(hInstance, hMainWnd);
	::ShowWindow(hMainWnd, nCmdShow);
	::UpdateWindow(hMainWnd);

	return(TRUE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    char dir = -1;
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_SIZE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:	
	case WM_CHAR:
	case WM_KEYUP:	
	case WM_IME_COMPOSITION:
		gGameFramework.OnProcessingWindowMessage(hWnd, message, wParam, lParam);
		break;
	case WM_KEYDOWN:
		gGameFramework.OnProcessingWindowMessage(hWnd, message, wParam, lParam);
		switch (wParam)
		{
		case VK_F10:
			//SwitchToBorderedMode(hWnd);
			break;

		case VK_F11:
			//SwitchToBorderlessMode(hWnd);
			break;
			
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			::DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			::DestroyWindow(hWnd);
			break;
		default:
			return(::DefWindowProc(hWnd, message, wParam, lParam));
		}
		break;
	case WM_PAINT:
		hdc = ::BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
	default:
		return(::DefWindowProc(hWnd, message, wParam, lParam));
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return((INT_PTR)TRUE);
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			::EndDialog(hDlg, LOWORD(wParam));
			return((INT_PTR)TRUE);
		}
		break;
	}
	return((INT_PTR)FALSE);
}

void SwitchToBorderedMode(HWND hWnd)
{
	// Use the desired border styles for bordered mode
	//DWORD dwStyle = WS_OVERLAPPEDWINDOW;

	// Set the new window style

	RECT rc = { 0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT };
	DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_BORDER;
	SetWindowLong(hWnd, GWL_STYLE, dwStyle);
	AdjustWindowRect(&rc, dwStyle, FALSE);
	// Adjust the window size and position to fit the bordered style
	//RECT rect;
	//GetClientRect(hWnd, &rect);
	//AdjustWindowRect(&rect, dwStyle, FALSE);

	//int width = rect.right - rect.left;
	//int height = rect.bottom - rect.top;

	//// Set the window position to the center of the screen
	//int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	//int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	//int x = (screenWidth - width) / 2;
	//int y = (screenHeight - height) / 2;

	//// Update the window size and position
	//SetWindowPos(hWnd, NULL, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

void SwitchToBorderlessMode(HWND hWnd)
{
	// Use WS_POPUP style for borderless mode
	DWORD dwStyle = WS_POPUP;

	// Set the new window style
	SetWindowLong(hWnd, GWL_STYLE, dwStyle);

	//// Set the window size and position to fill the screen
	//int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	//int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	//// Update the window size and position
	//SetWindowPos(hWnd, NULL, 0, 0, screenWidth, screenHeight, SWP_NOZORDER | SWP_NOACTIVATE);
}

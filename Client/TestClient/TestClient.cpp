#define _CRT_SECURE_NO_WARNINGS
#include "pch.h"
#include "framework.h"
#include "TestClient.h"
#include "OverlapEx.h"
using namespace std;

#define MAX_LOADSTRING 100

HINSTANCE hInst;                          
WCHAR szTitle[MAX_LOADSTRING];                 
WCHAR szWindowClass[MAX_LOADSTRING];           

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);


SOCKET s;
LPFN_DISCONNECTEX LpfnDisconnectex;
LPFN_CONNECTEX LpfnConnectex;
HANDLE iocp;
test_client::OverlapEx g_over;
int g_remainData;
bool g_match = false;
bool g_lobby = false;
char name[20];

namespace test_client {
    void InitNetwork()
    {
        std::wcout.imbue(std::locale("korean"));
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);


        s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (INVALID_SOCKET == s) {
            std::cout << "Create Fail" << std::endl;
        }
        int optval = 1;      
        
        GUID op = WSAID_DISCONNECTEX;
        DWORD bytes = 0;
        WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &op, sizeof(op), &LpfnDisconnectex, sizeof(LpfnDisconnectex), &bytes, NULL, NULL);
        
        GUID op2 = WSAID_CONNECTEX;
        WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &op2, sizeof(op2), &LpfnConnectex, sizeof(LpfnConnectex), &bytes, NULL, NULL);

        SOCKADDR_IN server_addr;
        ZeroMemory(&server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(LOBBY_PORT);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

        SOCKADDR_IN cl;
        memset(&cl, 0, sizeof(cl));
        cl.sin_family = AF_INET;
        cl.sin_port = 0;
        cl.sin_addr.S_un.S_addr = INADDR_ANY;

        bind(s, reinterpret_cast<LPSOCKADDR>(&cl), sizeof(cl));

        WSAConnect(s, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr), NULL, NULL, NULL, NULL);
        iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, NULL, 0);
        CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), iocp, 0, 0);
    }

    void SendPacket(void* packet)
    {
        OverlapEx* over = new OverlapEx(reinterpret_cast<char*>(packet));
        WSASend(s, &over->GetWSA(), 1, 0, 0, &over->GetOver(), 0);
        delete packet;
    }

    void RecvPacket()
    {
        DWORD recv_flag = 0;
        memset(&g_over.GetOver(), 0, sizeof(g_over.GetOver()));
        g_over.GetWSA().len = BUF_SIZE - g_remainData;
        g_over.GetWSA().buf = g_over.GetSendBuf() + g_remainData;
        int byte = WSARecv(s, &g_over.GetWSA(), 1, 0, &recv_flag, &g_over.GetOver(), 0);
    }

    void Packet_Exec(int id, char* packet)
    {
        switch (packet[1]) {
        case SC_LOGIN_INFO:
            cout << "Login!" << endl;
            break;
        case SC_MOVE_PLAYER:
            cout << "Move Player!" << endl;
            break;
        case SC_MATCH_PLAYER: {
            cout << "Match" << endl;
            SC_MATCH_PACKET* p = reinterpret_cast<SC_MATCH_PACKET*>(packet);                    

            OverlapEx* over2 = new OverlapEx();
            over2->ResetOver();
            over2->SetOP(OP_TYPE::OP_DISCONNECT);
            over2->SetInfo1(p->gameport);
            over2->SetInfo2(p->gameip);
            if (false == LpfnDisconnectex(s, &over2->GetOver(), TF_REUSE_SOCKET, NULL) && WSA_IO_PENDING != WSAGetLastError() && ERROR_IO_PENDING != WSAGetLastError()) {
                WCHAR* mess;
                int errorNum = GetLastError();
                cout << errorNum << endl;
                FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, errorNum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&mess, 0, NULL);

                std::wcout << L"  에러 => " << mess << std::endl;
                while (true);
                LocalFree(mess);
                delete over2;
            }
        }
            break;
        case SC_MATCH_END: {
            cout << "Match End" << endl;
            SC_MATCH_END_PACKET* p = reinterpret_cast<SC_MATCH_END_PACKET*>(packet);
            
            OverlapEx* over2 = new OverlapEx();
            over2->ResetOver();
            over2->SetOP(OP_TYPE::OP_DISCONNECT);
            over2->SetInfo1(p->lobbyport);
            over2->SetInfo2(p->lobbyip);
            if (false == LpfnDisconnectex(s, &over2->GetOver(), TF_REUSE_SOCKET, NULL) && WSA_IO_PENDING != WSAGetLastError() && ERROR_IO_PENDING != WSAGetLastError()) {
                WCHAR* mess;
                int errorNum = GetLastError();
                cout << errorNum << endl;
                FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, errorNum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&mess, 0, NULL);

                std::wcout << L"  에러 => " << mess << std::endl;
                while (true);
                LocalFree(mess);
                delete over2;
            }
            g_match = false;

            break;
        }
        case SC_CHAT: {
            SC_CHAT_PACKET* p = reinterpret_cast<SC_CHAT_PACKET*>(packet);
            wcout << "Client " << p->id << " Sent: " << p->chat << endl;
            break;
        }
        default:
            cout << "Packet Err" << endl;
            break;
        }
    }

    void IOCP_Exec()
    {
        RecvPacket();
        while (true) {
            DWORD bytes;
            ULONG_PTR key;
            WSAOVERLAPPED* over = nullptr;
            int err = GetQueuedCompletionStatus(iocp, &bytes, &key, &over, INFINITE);
            OverlapEx* over_ex = reinterpret_cast<OverlapEx*>(over);
            int id = static_cast<int>(key);

            switch (over_ex->GetOP()) {
            case OP_TYPE::OP_RECV: {
                int remaindata = bytes + g_remainData;
                char* p = over_ex->GetSendBuf();
                while (remaindata > 0) {
                    int p_size = p[0];
                    if (p_size <= remaindata) {
                        Packet_Exec(id, p);
                        p = p + p_size;
                        remaindata = remaindata - p_size;
                    }
                    else break;
                }
                g_remainData = remaindata;
                if (remaindata > 0)
                    memcpy(over_ex->GetSendBuf(), p, remaindata);
                RecvPacket();
                break;
            }
            case OP_TYPE::OP_SEND:
                delete over_ex;
                break;

            case OP_TYPE::OP_DISCONNECT: {
                cout << "Disconnect" << endl;


                SOCKADDR_IN server_addr;
                ZeroMemory(&server_addr, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(over_ex->GetInfo1());
                inet_pton(AF_INET, over_ex->GetInfo2(), &server_addr.sin_addr);

                OverlapEx* over = new OverlapEx();
                over->ResetOver();
                over->SetOP(OP_TYPE::OP_CONNECT);
                if (false == LpfnConnectex(s, reinterpret_cast<LPSOCKADDR>(&server_addr), sizeof(server_addr), NULL, 0, NULL, &over->GetOver()) && WSA_IO_PENDING != WSAGetLastError() && ERROR_IO_PENDING != WSAGetLastError()) {
                    WCHAR* mess;
                    int errorNum = GetLastError();
                    cout << errorNum << endl;
                    FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL, errorNum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&mess, 0, NULL);

                    std::wcout << L"  에러 => " << mess << std::endl;
                    while (true);
                    LocalFree(mess);
                }
                delete over_ex;
                break;
            }

            case OP_TYPE::OP_CONNECT:
                cout << "connect" << endl;
                g_lobby != g_lobby;

                //Connect to Game Server
                if (false == g_lobby) {
                    CS_LOGIN_PACKET* packet = new CS_LOGIN_PACKET;
                    packet->size = sizeof(CS_LOGIN_PACKET);
                    packet->type = CS_LOGIN;
                    memcpy_s(packet->name, 20, name, 20);
                    SendPacket(packet);
                }
                delete over_ex;
                break;
            }

        }
    }
}

uniform_int_distribution<> uid;
random_device rd;
default_random_engine dre(rd());

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    AllocConsole();
    freopen("CONOUT$", "wt", stdout);    

    test_client::InitNetwork();
    thread th{ test_client::IOCP_Exec };
    for (int i = 0; i < 10; ++i) {
        name[i] = 'A' + uid(dre) % 26;
    }
    name[10] = '\n';

    CS_LOGIN_PACKET* packet = new CS_LOGIN_PACKET;
    packet->size = sizeof(CS_LOGIN_PACKET);
    packet->type = CS_LOGIN;
    memcpy_s(packet->name, 20, name, 20);
    test_client::SendPacket(packet);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TESTCLIENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTCLIENT));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    th.join();
    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTCLIENT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TESTCLIENT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      0, 0, 400, 50, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    char dir = -1;
    switch (message)
    {
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_LEFT: {
            dir = 0;
            cout << "Left" << endl;
            break;
        }
        case VK_RIGHT: {
            dir = 1;
            cout << "Right" << endl;
            break;
        }
        case VK_UP: {
            dir = 2;
            cout << "Up" << endl;
            break;
        }
        case VK_DOWN: {
            dir = 3;
            cout << "Down" << endl;
            break;
        }
        case 'g':
        case 'G': {
            CS_MATCH_PACKET* p = new CS_MATCH_PACKET;
            p->size = sizeof(p);
            p->type = CS_MATCH;
            p->match = !g_match;
            p->character = 0;
            p->match_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            g_match = !g_match;
            test_client::SendPacket(p);
            break;
        }
        case 'h':
        case 'H': {
            CS_MATCH_PACKET* p = new CS_MATCH_PACKET;
            p->size = sizeof(p);
            p->type = CS_MATCH;
            p->match = !g_match;
            p->character = 1;
            p->match_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            g_match = !g_match;
            test_client::SendPacket(p);
            break;
        }
        case 'a':
        case 'A': {
            CS_MATCH_END_PACKET* p = new CS_MATCH_END_PACKET;
            p->size = sizeof(p);
            p->type = CS_MATHCH_END;
            test_client::SendPacket(p);
            break;
        }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        closesocket(s);
        WSACleanup();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    if (dir != -1) {
        CS_MOVE_PACKET* p = new CS_MOVE_PACKET;
        p->size = sizeof(p);
        p->type = CS_MOVE;
        p->direction = dir;
        test_client::SendPacket(reinterpret_cast<char*>(p));        
    }
    return 0;
}

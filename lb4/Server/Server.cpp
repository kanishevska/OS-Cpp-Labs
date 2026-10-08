#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string>

#pragma comment(lib, "WS2_32.lib")

#define SERV_PORT 5000
#define WSA_ACCEPT (WM_USER + 0)
#define WSA_NETEVENT (WM_USER + 1)

HWND hEdit = NULL;
SOCKET srv_socket = INVALID_SOCKET;
SOCKET client_sockets[2] = { INVALID_SOCKET, INVALID_SOCKET };
SOCKADDR_IN client_addrs[2];
int client_count = 0;

void LogMessage(const char* msg) {
    int len = GetWindowTextLengthA(hEdit);
    SendMessageA(hEdit, EM_SETSEL, len, len);
    SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)msg);
    SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)"\r\n--------------------\r\n");
}

void StartServer(HWND hWnd) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        MessageBoxA(hWnd, "Помилка ініціалізації Winsock!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_socket == INVALID_SOCKET) {
        MessageBoxA(hWnd, "Помилка створення сокета сервера!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    SOCKADDR_IN srv_address;
    srv_address.sin_family = AF_INET;
    srv_address.sin_port = htons(SERV_PORT);
    srv_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(srv_socket, (LPSOCKADDR)&srv_address, sizeof(srv_address)) == SOCKET_ERROR) {
        closesocket(srv_socket);
        MessageBoxA(hWnd, "Помилка прив'язки до порту!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (listen(srv_socket, 2) == SOCKET_ERROR) {
        closesocket(srv_socket);
        MessageBoxA(hWnd, "Помилка режиму прослуховування!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    WSAAsyncSelect(srv_socket, hWnd, WSA_ACCEPT, FD_ACCEPT);
    LogMessage("Сервер успішно запущено. Очікування підключень клієнтів...");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        hEdit = CreateWindowA("EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 10, 460, 340, hWnd, NULL, NULL, NULL);
        StartServer(hWnd);
        break;
    }
    case WSA_ACCEPT: {
        if (WSAGETSELECTERROR(lParam)) break;

        if (client_count >= 2) {
            SOCKET tempSock = accept(srv_socket, NULL, NULL);
            closesocket(tempSock);
            break;
        }

        int addr_len = sizeof(client_addrs[client_count]);
        client_sockets[client_count] = accept(srv_socket, (LPSOCKADDR)&client_addrs[client_count], &addr_len);

        if (client_sockets[client_count] != INVALID_SOCKET) {
            WSAAsyncSelect(client_sockets[client_count], hWnd, WSA_NETEVENT, FD_READ | FD_CLOSE);
            char buf[128];
            sprintf_s(buf, "Підключено Клієнта №%d", client_count + 1);
            LogMessage(buf);
            client_count++;
        }
        break;
    }
    case WSA_NETEVENT: {
        SOCKET sock = (SOCKET)wParam;
        int clientNum = (sock == client_sockets[0]) ? 1 : 2;

        if (WSAGETSELECTEVENT(lParam) == FD_READ) {
            char buf[512] = { 0 };
            int bytesReceived = recv(sock, buf, sizeof(buf) - 1, 0);
            if (bytesReceived > 0) {
                buf[bytesReceived] = '\0';
                char logBuf[600];
                sprintf_s(logBuf, "Прийнято дані від Клієнта №%d:\r\n%s", clientNum, buf);
                LogMessage(logBuf);
            }
        }
        else if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) {
            closesocket(sock);
            char buf[128];
            sprintf_s(buf, "Клієнт №%d відключився.", clientNum);
            LogMessage(buf);
        }
        break;
    }
    case WM_DESTROY:
        for (int i = 0; i < 2; i++) {
            if (client_sockets[i] != INVALID_SOCKET) closesocket(client_sockets[i]);
        }
        if (srv_socket != INVALID_SOCKET) closesocket(srv_socket);
        WSACleanup();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "ServerLab4Class";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassA(&wc);

    HWND hWnd = CreateWindowA("ServerLab4Class", "Lab 4 - Server (Sockets)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 500, 400, NULL, NULL, hInstance, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
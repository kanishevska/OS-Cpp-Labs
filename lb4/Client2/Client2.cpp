#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "WS2_32.lib")

#define SERV_PORT 5000
#define ID_BTN_CONNECT 101
#define ID_BTN_SEND 102

SOCKET cln_socket = INVALID_SOCKET;
HWND hEdit = NULL;

void LogMsg(const char* msg) {
    int len = GetWindowTextLengthA(hEdit);
    SendMessageA(hEdit, EM_SETSEL, len, len);
    SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)msg);
    SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
}

void ConnectToServer(HWND hWnd) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    cln_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cln_socket == INVALID_SOCKET) {
        LogMsg("Помилка створення сокета!");
        return;
    }

    SOCKADDR_IN dest_sin;
    dest_sin.sin_family = AF_INET;
    dest_sin.sin_port = htons(SERV_PORT);
    dest_sin.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(cln_socket, (PSOCKADDR)&dest_sin, sizeof(dest_sin)) == SOCKET_ERROR) {
        LogMsg("Помилка підключення до сервера!");
        closesocket(cln_socket);
        cln_socket = INVALID_SOCKET;
        return;
    }

    LogMsg("Успішно підключено до сервера!");
}

void SendData2(HWND hWnd) {
    if (cln_socket == INVALID_SOCKET) {
        LogMsg("Спочатку встановіть з'єднання!");
        return;
    }

    // Збір метрик для Клієнта 2 (Варіант 4)
    int cySmCaption = GetSystemMetrics(SM_CYSMCAPTION);
    int cxFixedFrame = GetSystemMetrics(SM_CXFIXEDFRAME);

    HDC hdc = GetDC(hWnd);
    int numColors = GetDeviceCaps(hdc, NUMCOLORS);
    ReleaseDC(hWnd, hdc);

    char sendBuf[256];
    sprintf_s(sendBuf, "Розмір шрифту/заголовка меню (SM_CYSMCAPTION): %d px\r\nШирина фіксованої рамки (SM_CXFIXEDFRAME): %d px\r\nКількість кольорів (NUMCOLORS): %d",
        cySmCaption, cxFixedFrame, numColors);

    if (send(cln_socket, sendBuf, (int)strlen(sendBuf), 0) != SOCKET_ERROR) {
        LogMsg("Дані Клієнта №2 успішно надіслано:");
        LogMsg(sendBuf);
    }
    else {
        LogMsg("Помилка надсилання даних!");
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateWindowA("BUTTON", "Підключитись", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 10, 130, 30, hWnd, (HMENU)ID_BTN_CONNECT, NULL, NULL);
        CreateWindowA("BUTTON", "Надіслати дані", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            150, 10, 130, 30, hWnd, (HMENU)ID_BTN_SEND, NULL, NULL);
        hEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_READONLY,
            10, 50, 360, 200, hWnd, NULL, NULL, NULL);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_CONNECT) ConnectToServer(hWnd);
        if (LOWORD(wParam) == ID_BTN_SEND) SendData2(hWnd);
        break;
    case WM_DESTROY:
        if (cln_socket != INVALID_SOCKET) closesocket(cln_socket);
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
    wc.lpszClassName = "Client2Lab4Class";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassA(&wc);

    HWND hWnd = CreateWindowA("Client2Lab4Class", "Lab 4 - Client 2",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 620, 430, 400, 300, NULL, NULL, hInstance, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
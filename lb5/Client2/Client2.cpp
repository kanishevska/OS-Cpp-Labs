#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#define ID_SEND_MESSAGE 32771

HINSTANCE hInst;
HWND hwndEdit;
LPCSTR ServerName = "\\\\.\\mailslot\\lab7";
char mess[2048] = "";
char szBuf[512] = "";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    hInst = hInstance;
    const wchar_t CLASS_NAME[] = L"Client2WindowClass";

    WNDCLASSW wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        0, CLASS_NAME, L"Клієнт #2",
        WS_OVERLAPPEDWINDOW,
        600, 380, 420, 260,
        NULL, NULL, hInstance, NULL
    );

    if (hWnd == NULL) return 0;

    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    AppendMenuW(hSubMenu, MF_STRING, ID_SEND_MESSAGE, L"Send Message");
    AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"MailSlot");
    SetMenu(hWnd, hMenu);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        hwndEdit = CreateWindowExW(
            0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            10, 10, 380, 170,
            hWnd, NULL, hInst, NULL
        );

        // Збір системних метрик за варіантом для Клієнта 2
        HDC hdc = GetDC(hWnd);
        int cyEdge = GetSystemMetrics(SM_CYEDGE);
        int cxEdge = GetSystemMetrics(SM_CXEDGE);
        int logPixelsX = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(hWnd, hdc);

        sprintf_s(mess, "Дані Клієнта #2:\r\n- висота рядка стану / 3D-рамки = %d\r\n- ширина панелі / 3D-рамки = %d\r\n- DPI горизонтально = %d",
            cyEdge, cxEdge, logPixelsX);

        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
        break;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        if (wmId == ID_SEND_MESSAGE)
        {
            DWORD cbWritten = 0;
            SendMessageA(hwndEdit, WM_GETTEXT, sizeof(szBuf), (LPARAM)szBuf);

            HANDLE hMailslot = CreateFileA(
                ServerName,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );

            if (hMailslot == INVALID_HANDLE_VALUE)
            {
                sprintf_s(mess, "%s\r\nПомилка поштового серверу: %d\r\n", szBuf, GetLastError());
                SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
                break;
            }

            WriteFile(hMailslot, szBuf, (DWORD)strlen(szBuf) + 1, &cbWritten, NULL);
            CloseHandle(hMailslot);
        }
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#define ID_SEND_MESSAGE 32771

HINSTANCE hInst;
HWND hwndEdit;
char mess[2048] = "";
char szBuf[512] = "";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    hInst = hInstance;
    const wchar_t CLASS_NAME[] = L"Client1PipeClass";

    WNDCLASSW wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        0, CLASS_NAME, L"Клієнт #1",
        WS_OVERLAPPEDWINDOW,
        600, 100, 420, 260,
        NULL, NULL, hInstance, NULL
    );

    if (hWnd == NULL) return 0;

    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    AppendMenuW(hSubMenu, MF_STRING, ID_SEND_MESSAGE, L"Send Message");
    AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"NamedPipe");
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

        // Збір системних метрик Варіанта №4 для Клієнта 1
        HDC hdc = GetDC(hWnd);
        int isServerR2 = GetSystemMetrics(SM_SERVERR2);
        int numMonitors = GetSystemMetrics(SM_CMONITORS);
        int bitsPixel = GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(hWnd, hdc);

        sprintf_s(mess, "Дані від Клієнта №1:\r\n- версія ОС Windows Server R2 = %s\r\n- кількість моніторів = %d\r\n- глибина кольору = %d bit",
            (isServerR2 ? "Так" : "Ні"), numMonitors, bitsPixel);

        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
        break;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        if (wmId == ID_SEND_MESSAGE)
        {
            SendMessageA(hwndEdit, WM_GETTEXT, sizeof(szBuf), (LPARAM)szBuf);

            // Підключення до іменованого каналу сервера
            HANDLE hPipe = CreateFileA(
                "\\\\.\\pipe\\lab7_pipe",
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
            );

            if (hPipe == INVALID_HANDLE_VALUE)
            {
                DWORD dwErr = GetLastError();
                char errStr[128];
                sprintf_s(errStr, "Помилка підключення до каналу: %lu", dwErr);
                MessageBoxA(hWnd, errStr, "Pipe Error", MB_OK | MB_ICONERROR);
                break;
            }

            DWORD bytesWritten = 0;
            BOOL fSuccess = WriteFile(
                hPipe,
                szBuf,
                (DWORD)strlen(szBuf) + 1,
                &bytesWritten,
                NULL
            );

            if (!fSuccess)
            {
                MessageBoxA(hWnd, "Помилка відправки даних", "Pipe Error", MB_OK | MB_ICONERROR);
            }

            CloseHandle(hPipe);
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
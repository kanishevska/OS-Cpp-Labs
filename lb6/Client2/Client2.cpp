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
    const wchar_t CLASS_NAME[] = L"Client2FileMappingClass";

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
    AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"FileMapping");
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

        // Збір системних метрик Варіанта №4 для Клієнта 2
        HDC hdc = GetDC(hWnd);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        int cxVScroll = GetSystemMetrics(SM_CXVSCROLL);
        int bitsPixel = GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(hWnd, hdc);

        sprintf_s(mess, "Дані від Клієнта №2:\r\n- розмір іконки = %dx%d px\r\n- ширина верт. смуги прокрутки = %d px\r\n- глибина кольору = %d bit",
            cxIcon, cyIcon, cxVScroll, bitsPixel);

        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
        break;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        if (wmId == ID_SEND_MESSAGE)
        {
            SendMessageA(hwndEdit, WM_GETTEXT, sizeof(szBuf), (LPARAM)szBuf);

            HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "myFileMapping");
            if (hMapFile == NULL)
            {
                MessageBoxA(hWnd, "Неможливо відкрити відображений у пам'яті об'єкт", "Map Error", MB_OK | MB_ICONERROR);
                break;
            }

            LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 512);
            if (pBuf == NULL)
            {
                MessageBoxA(hWnd, "Подання проектованого файлу неможливе", "Map Error", MB_OK | MB_ICONERROR);
                CloseHandle(hMapFile);
                break;
            }

            CopyMemory(pBuf, szBuf, strlen(szBuf) + 1);

            UnmapViewOfFile(pBuf);
            CloseHandle(hMapFile);

            HANDLE hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, "myFileMappingEvent");
            if (hEvent != NULL)
            {
                SetEvent(hEvent);
                CloseHandle(hEvent);
            }
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
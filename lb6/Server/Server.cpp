#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define ID_FILEMAPPING_START 32771
#define ID_FILEMAPPING_READ  32772
#define ID_FILEMAPPING_STOP  32773

// Глобальні змінні
HINSTANCE hInst;
HWND hwndEdit;
HANDLE hMapFile = NULL;
HANDLE hEvent = NULL;
HANDLE hWatchThread = NULL;
BOOL bKeepWatching = FALSE;

char mess[4096] = "";

// Прототипи
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI WatchThreadProc(LPVOID lpParam);

// Фоновий потік для автоматичного виявлення змін (Завдання "з зірочкою")
DWORD WINAPI WatchThreadProc(LPVOID lpParam)
{
    HWND hWnd = (HWND)lpParam;
    while (bKeepWatching)
    {
        if (hEvent != NULL)
        {
            // Очікування події від клієнта 100 мс
            DWORD dwWaitResult = WaitForSingleObject(hEvent, 100);
            if (dwWaitResult == WAIT_OBJECT_0)
            {
                // Читання з проекції файлу
                if (hMapFile != NULL)
                {
                    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 512);
                    if (pBuf != NULL)
                    {
                        sprintf_s(mess, "%s\r\n[Авто-виявлення]:\r\n%s\r\n", mess, (char*)pBuf);
                        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
                        UnmapViewOfFile(pBuf);
                    }
                }
                ResetEvent(hEvent);
            }
        }
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    hInst = hInstance;
    const wchar_t CLASS_NAME[] = L"ServerFileMappingClass";

    WNDCLASSW wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        0, CLASS_NAME, L"Сервер (FileMapping)",
        WS_OVERLAPPEDWINDOW,
        100, 100, 480, 560,
        NULL, NULL, hInstance, NULL
    );

    if (hWnd == NULL) return 0;

    // Створення програмного меню
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    AppendMenuW(hSubMenu, MF_STRING, ID_FILEMAPPING_START, L"Start");
    AppendMenuW(hSubMenu, MF_STRING, ID_FILEMAPPING_READ, L"Read");
    AppendMenuW(hSubMenu, MF_STRING, ID_FILEMAPPING_STOP, L"Stop");
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
        hwndEdit = CreateWindowExW(
            0, L"EDIT", L"Лабораторна робота - Відображення (проекція) файлу в пам'ять.\r\n",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            20, 10, 420, 460,
            hWnd, NULL, hInst, NULL
        );
        break;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case ID_FILEMAPPING_START:
            // Створення проекції файлу в оперативній пам'яті
            hMapFile = CreateFileMappingA(
                INVALID_HANDLE_VALUE,
                NULL,
                PAGE_READWRITE,
                0,
                512,
                "myFileMapping"
            );

            if (hMapFile == NULL)
            {
                MessageBoxA(hWnd, "Неможливо створити об'єкт відображення файлу", "Помилка", MB_OK | MB_ICONERROR);
                break;
            }

            // Створення події для авто-виявлення запису (Завдання *)
            hEvent = CreateEventA(NULL, TRUE, FALSE, "myFileMappingEvent");

            sprintf_s(mess, "Сервер FileMapping запущено\r\n");
            SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);

            // Запуск фонового потоку спостереження за подією
            bKeepWatching = TRUE;
            hWatchThread = CreateThread(NULL, 0, WatchThreadProc, hWnd, 0, NULL);
            break;

        case ID_FILEMAPPING_READ:
            if (hMapFile == NULL)
            {
                MessageBoxA(hWnd, "Сервер не запущено!", "Увага", MB_OK | MB_ICONWARNING);
                break;
            }

            {
                LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 512);
                if (pBuf == NULL)
                {
                    MessageBoxA(hWnd, "Подання проектованого файлу неможливе", "Помилка", MB_OK | MB_ICONERROR);
                    break;
                }

                sprintf_s(mess, "%s\r\n[Ручне читання]:\r\n%s\r\n", mess, (char*)pBuf);
                SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
                UnmapViewOfFile(pBuf);
            }
            break;

        case ID_FILEMAPPING_STOP:
            bKeepWatching = FALSE;
            if (hWatchThread != NULL)
            {
                WaitForSingleObject(hWatchThread, 1000);
                CloseHandle(hWatchThread);
                hWatchThread = NULL;
            }
            if (hEvent != NULL)
            {
                CloseHandle(hEvent);
                hEvent = NULL;
            }
            if (hMapFile != NULL)
            {
                CloseHandle(hMapFile);
                hMapFile = NULL;
            }

            sprintf_s(mess, "%s\r\nСервер FileMapping зупинено\r\n", mess);
            SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    case WM_DESTROY:
        bKeepWatching = FALSE;
        if (hWatchThread != NULL) CloseHandle(hWatchThread);
        if (hEvent != NULL) CloseHandle(hEvent);
        if (hMapFile != NULL) CloseHandle(hMapFile);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
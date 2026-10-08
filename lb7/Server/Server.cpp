#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define ID_PIPE_START 32771
#define ID_PIPE_STOP  32772

// Глобальні змінні
HINSTANCE hInst;
HWND hwndEdit;
HANDLE hPipe = INVALID_HANDLE_VALUE;
HANDLE hServerThread = NULL;
BOOL bServerRunning = FALSE;

char mess[4096] = "";

// Прототипи
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI ServerThreadProc(LPVOID lpParam);

// Фоновий потік для асинхронної прийому даних від клієнтів через Named Pipe
DWORD WINAPI ServerThreadProc(LPVOID lpParam)
{
    HWND hWnd = (HWND)lpParam;
    char szBuffer[512];
    DWORD bytesRead = 0;

    while (bServerRunning)
    {
        // Створення екземпляра Іменованого Каналу (Named Pipe)
        hPipe = CreateNamedPipeA(
            "\\\\.\\pipe\\lab7_pipe",
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            512, 512,
            0,
            NULL
        );

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            Sleep(100);
            continue;
        }

        // Очікування підключення клієнта
        BOOL fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (fConnected && bServerRunning)
        {
            // Зчитування даних від клієнта
            BOOL fSuccess = ReadFile(
                hPipe,
                szBuffer,
                sizeof(szBuffer) - 1,
                &bytesRead,
                NULL
            );

            if (fSuccess && bytesRead > 0)
            {
                szBuffer[bytesRead] = '\0';
                sprintf_s(mess, "%s\r\n%s\r\n", mess, szBuffer);
                SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
            }
        }

        // Відключення та закриття поточного дескриптора каналу
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    hInst = hInstance;
    const wchar_t CLASS_NAME[] = L"ServerPipeClass";

    WNDCLASSW wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        0, CLASS_NAME, L"Сервер (Named Pipes)",
        WS_OVERLAPPEDWINDOW,
        100, 100, 480, 560,
        NULL, NULL, hInstance, NULL
    );

    if (hWnd == NULL) return 0;

    // Створення програмного меню
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    AppendMenuW(hSubMenu, MF_STRING, ID_PIPE_START, L"Start");
    AppendMenuW(hSubMenu, MF_STRING, ID_PIPE_STOP, L"Stop");
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
        hwndEdit = CreateWindowExW(
            0, L"EDIT", L"Лабораторна робота - Іменовані канали (Named Pipes).\r\n",
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
        case ID_PIPE_START:
            if (bServerRunning) break;

            bServerRunning = TRUE;
            sprintf_s(mess, "Сервер Named Pipe запущено\r\n");
            SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);

            // Запуск фонового потоку очікування клієнтів
            hServerThread = CreateThread(NULL, 0, ServerThreadProc, hWnd, 0, NULL);
            break;

        case ID_PIPE_STOP:
            if (!bServerRunning) break;

            bServerRunning = FALSE;

            {
                // Підключення-заглушка для розблокування ConnectNamedPipe у фоновому потоці
                HANDLE hDummyPipe = CreateFileA(
                    "\\\\.\\pipe\\lab7_pipe",
                    GENERIC_WRITE,
                    0, NULL, OPEN_EXISTING, 0, NULL
                );
                if (hDummyPipe != INVALID_HANDLE_VALUE) CloseHandle(hDummyPipe);
            }

            if (hServerThread != NULL)
            {
                WaitForSingleObject(hServerThread, 1000);
                CloseHandle(hServerThread);
                hServerThread = NULL;
            }

            sprintf_s(mess, "%s\r\nСервер Named Pipe зупинено\r\n", mess);
            SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    case WM_DESTROY:
        bServerRunning = FALSE;
        if (hServerThread != NULL) CloseHandle(hServerThread);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
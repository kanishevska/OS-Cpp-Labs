#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define IDT_TIMER 1001
#define ID_MAILSLOT_CREATE 32771
#define ID_MAILSLOT_CLOSE  32772

// Глобальні змінні
HINSTANCE hInst;
HWND hwndEdit;
HANDLE hMailslot = INVALID_HANDLE_VALUE;
LPSTR MailslotName = (LPSTR)"\\\\.\\mailslot\\lab7";
char mess[4096] = "";
char szBuf[512] = "";

// Прототип віконної функції
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    hInst = hInstance;
    const wchar_t CLASS_NAME[] = L"ServerWindowClass";

    WNDCLASSW wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        0, CLASS_NAME, L"Поштовий Сервер (Server)",
        WS_OVERLAPPEDWINDOW,
        100, 100, 480, 560,
        NULL, NULL, hInstance, NULL
    );

    if (hWnd == NULL) return 0;

    // Створення меню програмно
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    AppendMenuW(hSubMenu, MF_STRING, ID_MAILSLOT_CREATE, L"Create");
    AppendMenuW(hSubMenu, MF_STRING, ID_MAILSLOT_CLOSE, L"Close");
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
        hwndEdit = CreateWindowExW(
            0, L"EDIT", L"Лабораторна робота - Поштова скриня.\r\n",
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
        case ID_MAILSLOT_CREATE:
            sprintf_s(mess, "Поштова скриня\r\n");
            SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);

            // Створення поштової скриньки
            hMailslot = CreateMailslotA(MailslotName, 0, MAILSLOT_WAIT_FOREVER, NULL);
            if (hMailslot == INVALID_HANDLE_VALUE)
            {
                sprintf_s(mess, "%s\r\nПоштовий сервер. Error: %d\r\n", mess, GetLastError());
                SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
                break;
            }

            sprintf_s(mess, "%s\r\nСервер запущено\r\n", mess);
            SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);

            // Запуск таймера перевірки скриньки кажі 1000 мс (1 сек)
            SetTimer(hWnd, IDT_TIMER, 1000, NULL);
            break;

        case ID_MAILSLOT_CLOSE:
            if (hMailslot != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hMailslot);
                hMailslot = INVALID_HANDLE_VALUE;
            }
            KillTimer(hWnd, IDT_TIMER);
            sprintf_s(mess, "%s\r\nПоштовий сервер закрито\r\n", mess);
            SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    case WM_TIMER:
        if (wParam == IDT_TIMER && hMailslot != INVALID_HANDLE_VALUE)
        {
            DWORD cbMessage = 0, cMessage = 0, cbRead = 0;
            BOOL fResult = GetMailslotInfo(hMailslot, NULL, &cbMessage, &cMessage, NULL);

            if (fResult && cMessage != 0)
            {
                if (ReadFile(hMailslot, szBuf, sizeof(szBuf) - 1, &cbRead, NULL))
                {
                    szBuf[cbRead] = '\0';
                    sprintf_s(mess, "%s\r\n%s\r\n", mess, szBuf);
                    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
                }
            }
        }
        break;

    case WM_DESTROY:
        if (hMailslot != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hMailslot);
        }
        KillTimer(hWnd, IDT_TIMER);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
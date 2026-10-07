#include <windows.h>
#include <vector>
#include <cmath>
#include <string>

struct ThreadInfo {
    HANDLE hThread;
    DWORD threadId;
    int x;
    int y;
    bool active;
    bool suspended; //*Прапорець паузи
    int counter;
    COLORREF color; //*Власний колір для кожного потоку
};

std::vector<ThreadInfo*> g_threads;
CRITICAL_SECTION g_cs;
HWND g_hWnd = NULL;

DWORD WINAPI ThreadProc(LPVOID lpParam) {
    ThreadInfo* info = (ThreadInfo*)lpParam;

    while (info->active) {
        if (!info->suspended) {
            info->counter++;
            InvalidateRect(g_hWnd, NULL, TRUE);
        }
        Sleep(200);
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        ThreadInfo* info = new ThreadInfo();
        info->x = x;
        info->y = y;
        info->active = true;
        info->suspended = false;
        info->counter = 0;

        // *Випадковий яскравий колір для тексту нового потоку
        info->color = RGB(rand() % 200, rand() % 200, rand() % 200);

        EnterCriticalSection(&g_cs);
        info->hThread = CreateThread(NULL, 0, ThreadProc, info, 0, &info->threadId);
        if (info->hThread != NULL) {
            g_threads.push_back(info);
        }
        else {
            delete info;
        }
        LeaveCriticalSection(&g_cs);
        break;
    }
    case WM_MBUTTONDOWN: { // *Пауза / Відновлення найближчого потоку на клик колесика
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        EnterCriticalSection(&g_cs);
        int closestIndex = -1;
        double minDistance = 1e9;

        for (size_t i = 0; i < g_threads.size(); ++i) {
            if (g_threads[i]->active) {
                double dist = std::sqrt(std::pow(g_threads[i]->x - mouseX, 2) +
                    std::pow(g_threads[i]->y - mouseY, 2));
                if (dist < minDistance) {
                    minDistance = dist;
                    closestIndex = (int)i;
                }
            }
        }

        if (closestIndex != -1) {
            ThreadInfo* t = g_threads[closestIndex];
            if (t->suspended) {
                ResumeThread(t->hThread); // *Відновлення
                t->suspended = false;
            }
            else {
                SuspendThread(t->hThread); // *Пауза
                t->suspended = true;
            }
            InvalidateRect(hWnd, NULL, TRUE);
        }
        LeaveCriticalSection(&g_cs);
        break;
    }
    case WM_RBUTTONDOWN: {
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        EnterCriticalSection(&g_cs);
        int closestIndex = -1;
        double minDistance = 1e9;

        for (size_t i = 0; i < g_threads.size(); ++i) {
            if (g_threads[i]->active) {
                double dist = std::sqrt(std::pow(g_threads[i]->x - mouseX, 2) +
                    std::pow(g_threads[i]->y - mouseY, 2));
                if (dist < minDistance) {
                    minDistance = dist;
                    closestIndex = (int)i;
                }
            }
        }

        if (closestIndex != -1) {
            g_threads[closestIndex]->active = false;
            // Якщо потік був на паузі, розблокуємо його, щоб він зміг завершити цикл
            if (g_threads[closestIndex]->suspended) {
                ResumeThread(g_threads[closestIndex]->hThread);
            }
            WaitForSingleObject(g_threads[closestIndex]->hThread, INFINITE);
            CloseHandle(g_threads[closestIndex]->hThread);
            delete g_threads[closestIndex];
            g_threads.erase(g_threads.begin() + closestIndex);
            InvalidateRect(hWnd, NULL, TRUE);
        }
        LeaveCriticalSection(&g_cs);
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        EnterCriticalSection(&g_cs);
        for (const auto& t : g_threads) {
            if (t->active) {
                SetTextColor(hdc, t->color); // *Встановлюємо унікальний колір
                SetBkMode(hdc, TRANSPARENT);

                std::wstring status = t->suspended ? L" [PAUSED]" : L"";
                std::wstring text = L"Pos: (" + std::to_wstring(t->x) + L"," +
                    std::to_wstring(t->y) + L") Val: " + std::to_wstring(t->counter) + status;

                TextOutW(hdc, t->x, t->y, text.c_str(), (int)text.length());
            }
        }
        LeaveCriticalSection(&g_cs);

        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        EnterCriticalSection(&g_cs);
        for (auto& t : g_threads) {
            t->active = false;
            if (t->suspended) ResumeThread(t->hThread);
            WaitForSingleObject(t->hThread, INFINITE);
            CloseHandle(t->hThread);
            delete t;
        }
        g_threads.clear();
        LeaveCriticalSection(&g_cs);

        DeleteCriticalSection(&g_cs);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int main() {
    InitializeCriticalSection(&g_cs);

    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"Lab3WindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    g_hWnd = CreateWindowW(L"Lab3WindowClass", L"Лабораторна 3 (Варіант 4)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 800, 600, NULL, NULL, hInstance, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
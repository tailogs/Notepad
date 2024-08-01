#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h> // Для системного трея
#include "resource.h"

#define VERSION "1.0.0"

HINSTANCE hInst;
HWND hEdit, hStatusBar;
NOTIFYICONDATA nid;

// Прототипы функций
void CreateMenus(HWND hwnd);
void CreateStatusBar(HWND hwnd);
void ShowAboutDialog(HWND hwnd);
void RemoveTrayIcon(HWND hwnd);

// Функция для обработки сообщений окна
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateMenus(hwnd);
            CreateStatusBar(hwnd);
            hEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                "EDIT",
                "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                0, 0, 0, 0,
                hwnd,
                NULL,
                hInst,
                NULL
            );
            SendMessage(hEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
            break;

        case WM_SIZE:
            {
                RECT rc;
                GetClientRect(hwnd, &rc);
                SetWindowPos(hEdit, NULL, 0, 0, rc.right, rc.bottom - 20, SWP_NOZORDER);
                SetWindowPos(hStatusBar, NULL, 0, rc.bottom - 20, rc.right, 20, SWP_NOZORDER);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_FILE_NEW:
                    SetWindowText(hEdit, "");
                    break;
                case ID_FILE_OPEN:
                    {
                        OPENFILENAME ofn;
                        CHAR szFile[260] = {0};

                        ZeroMemory(&ofn, sizeof(ofn));
                        ofn.lStructSize = sizeof(ofn);
                        ofn.hwndOwner = hwnd;
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = sizeof(szFile);
                        ofn.lpstrFilter = "Text files\0*.TXT\0All files\0*.*\0";
                        ofn.nFilterIndex = 1;
                        ofn.lpstrFileTitle = NULL;
                        ofn.nMaxFileTitle = 0;
                        ofn.lpstrInitialDir = NULL;
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                        if (GetOpenFileName(&ofn)) {
                            HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                            if (hFile != INVALID_HANDLE_VALUE) {
                                DWORD dwSize = GetFileSize(hFile, NULL);
                                if (dwSize != INVALID_FILE_SIZE) {
                                    CHAR *pBuffer = (CHAR *)GlobalAlloc(GPTR, dwSize + 1);
                                    if (pBuffer) {
                                        DWORD dwRead;
                                        if (ReadFile(hFile, pBuffer, dwSize, &dwRead, NULL)) {
                                            pBuffer[dwSize] = '\0';
                                            SetWindowText(hEdit, pBuffer);
                                        }
                                        GlobalFree(pBuffer);
                                    }
                                }
                                CloseHandle(hFile);
                            }
                        }
                    }
                    break;
                case ID_FILE_SAVEAS:
                    {
                        OPENFILENAME ofn;
                        CHAR szFile[260] = {0};

                        ZeroMemory(&ofn, sizeof(ofn));
                        ofn.lStructSize = sizeof(ofn);
                        ofn.hwndOwner = hwnd;
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = sizeof(szFile);
                        ofn.lpstrFilter = "Text files\0*.TXT\0All files\0*.*\0";
                        ofn.nFilterIndex = 1;
                        ofn.lpstrFileTitle = NULL;
                        ofn.nMaxFileTitle = 0;
                        ofn.lpstrInitialDir = NULL;
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

                        if (GetSaveFileName(&ofn)) {
                            HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                            if (hFile != INVALID_HANDLE_VALUE) {
                                DWORD dwSize = GetWindowTextLength(hEdit);
                                CHAR *pBuffer = (CHAR *)GlobalAlloc(GPTR, dwSize + 1);
                                if (pBuffer) {
                                    GetWindowText(hEdit, pBuffer, dwSize + 1);
                                    DWORD dwWritten;
                                    WriteFile(hFile, pBuffer, dwSize, &dwWritten, NULL);
                                    GlobalFree(pBuffer);
                                }
                                CloseHandle(hFile);
                            }
                        }
                    }
                    break;
                case ID_FILE_EXIT:
                    PostQuitMessage(0);
                    break;
                case ID_HELP_ABOUT:
                    ShowAboutDialog(hwnd);
                    break;
                case ID_EDIT_UNDO:
                    SendMessage(hEdit, EM_UNDO, 0, 0);
                    break;
                case ID_EDIT_CUT:
                    SendMessage(hEdit, WM_CUT, 0, 0);
                    break;
                case ID_EDIT_COPY:
                    SendMessage(hEdit, WM_COPY, 0, 0);
                    break;
                case ID_EDIT_PASTE:
                    SendMessage(hEdit, WM_PASTE, 0, 0);
                    break;
            }
            break;

        case WM_USER + 1: // Сообщение из трея
            if (lParam == WM_RBUTTONUP) {
                HMENU hTrayMenu = CreatePopupMenu();
                AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_SHOW, "Show");
                AppendMenu(hTrayMenu, MF_STRING, ID_FILE_EXIT, "Exit");

                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hTrayMenu);
            } else if (lParam == WM_LBUTTONDBLCLK) {
                ShowWindow(hwnd, SW_RESTORE); // Восстановление окна
                SetForegroundWindow(hwnd); // Активировать окно
            }
            break;

        case WM_CLOSE:
            // Скрытие окна вместо его закрытия
            ShowWindow(hwnd, SW_HIDE);
            return 0;

        case WM_DESTROY:
            RemoveTrayIcon(hwnd); // Удаление иконки трея
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Функция для добавления иконки в системный трей
void AddTrayIcon(HWND hwnd) {
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = IDI_ICON1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
    strcpy(nid.szTip, "Simple Notepad");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

// Функция для удаления иконки из системного трея
void RemoveTrayIcon(HWND hwnd) {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

// Основная функция приложения
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HWND myConsole = GetConsoleWindow(); //window handle
    ShowWindow(myConsole, 0); //handle window
    
    WNDCLASSEX wcex;
    HWND hwnd;
    MSG msg;

    hInst = hInstance;

    // Регистрация класса окна
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MYMENU);
    wcex.lpszClassName = "NotepadClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));

    if (!RegisterClassEx(&wcex)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Создание окна
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "NotepadClass",
        "Simple Notepad",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    AddTrayIcon(hwnd); // Добавление иконки в трей

    // Основной цикл сообщений
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// Функция для создания строки состояния
void CreateStatusBar(HWND hwnd) {
    hStatusBar = CreateWindowEx(
        0,
        STATUSCLASSNAME,
        NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        hwnd,
        NULL,
        hInst,
        NULL
    );

    if (hStatusBar) {
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)"Ready");
    }
}

// Функция для отображения диалога "О программе"
void ShowAboutDialog(HWND hwnd) {
    char message[256];
    sprintf(message, "Notepad %s\nA simple notepad.\nDeveloper - Taillogs.", VERSION);
    MessageBox(hwnd, message, "About Simple Notepad", MB_OK | MB_ICONINFORMATION);
}

// Функция для создания меню
void CreateMenus(HWND hwnd) {
    HMENU hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MYMENU));
    SetMenu(hwnd, hMenu);
}

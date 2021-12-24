// Editor.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "framework.h"
#include "Editor.h"


#include <fstream>
#include <thread>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name


Editor editor;


enum Hotkeys
{
    UNUSED = 0,
    PRINTSCREEN = 1,
};

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_EDITOR, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EDITOR));

    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {

            editor.Run();

        }
    }
   

    return (int) msg.wParam;
}



ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EDITOR));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    int x = CW_USEDEFAULT, y = 0, w = CW_USEDEFAULT, h = 0;
    bool fullscreen = false;
    bool borderless = false;
    bool allow_hdr = true;
    std::string voidStr = "";

    std::ifstream file("config.ini");
    if (file.is_open())
    {
        int enabled;
        file >> voidStr >> enabled;
        if (enabled != 0)
        {
            file >> voidStr >> x >> voidStr >> y >> voidStr >> w >> voidStr >> h >> voidStr >> fullscreen >> voidStr >> borderless >> voidStr >> allow_hdr;
            editor.allow_hdr = allow_hdr;
        }
    }
    file.close();

    HWND hWnd = NULL;

    if (borderless)
    {
        hWnd = CreateWindowEx(WS_EX_APPWINDOW,
            szWindowClass,
            szTitle,
            WS_POPUP,
            x, y, w, h,
            NULL,
            NULL,
            hInstance,
            NULL
        );
    }
    else
    {
        hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
            x, y, w, h, NULL, NULL, hInstance, NULL);
    }

    if (!hWnd)
    {
        return FALSE;
    }

    editor.SetWindow(hWnd, fullscreen);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    RegisterHotKey(hWnd, PRINTSCREEN, 0, VK_SNAPSHOT);

    return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
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

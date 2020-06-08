// DeviceChangeTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

#define ID_LABEL 0

HWND hwndlabel = NULL;

#ifndef DPRINT1
int DPRINT1(const char* format, ...)
{
    va_list arg;
    int done;

    va_start(arg, format);
    done = vfprintf(stdout, format, arg);
    va_end(arg);

    return done;
}
#endif

static BOOL
RegisterDevNotificationForHwnd(
    IN HWND hWnd,
    OUT HDEVNOTIFY* hDeviceNotify)
{
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter = { 0 };
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_HDR);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

    *hDeviceNotify = RegisterDeviceNotification(hWnd,
        &NotificationFilter,
        DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);

    return *hDeviceNotify != NULL;
}

static BOOLEAN
GetFriendlyDeviceName(
    IN PDEV_BROADCAST_DEVICEINTERFACE pDevice,
    OUT LPCWSTR* pszName)
{
    BOOLEAN result = FALSE, DeviceInterfaceDataInitialized = FALSE;
    HDEVINFO hList = NULL;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData = { 0 };

    if (!pDevice 
        || ~pDevice->dbcc_devicetype & DBT_DEVTYP_DEVICEINTERFACE 
        || !pDevice->dbcc_name)
    {
        DPRINT1("pDevice is not initialized properly\n");
        goto cleanup;
    }
    
    /* Create an empty device info set */
    hList = SetupDiCreateDeviceInfoList(NULL, 0);
    if (!hList)
    {
        DPRINT1("SetupDiCreateDeviceInfoList failed (error %d)\n", GetLastError());
        goto cleanup;
    }

    /* Set up DeviceInterfaceData */
    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    /* Open device interface */
    DPRINT1("Open device interface\n"); // remove this line
    DeviceInterfaceDataInitialized = SetupDiOpenDeviceInterface(hList,
        &pDevice->dbcc_name[0],
        0,
        &DeviceInterfaceData);
    DPRINT1("dbcc_devicetype: %d\n", pDevice->dbcc_devicetype);
    if (!DeviceInterfaceDataInitialized)
    {
        DPRINT1("SetupDiOpenDeviceInterface failed (error %d)\n", GetLastError());
        goto cleanup;
    }

    for (INT index = 0; ; index++)
    {
        SP_DEVINFO_DATA devInfo = { 0 };

        /* Variables for SetupDiGetDeviceRegistryProperty */
        TCHAR szName[1024];
        DWORD PropertyRegDataType = 0, RequiredSize = 0;

        /* Init devInfo */
        devInfo.cbSize = sizeof(SP_DEVINFO_DATA);

        /* Enumerate through hList */
        if (SetupDiEnumDeviceInfo(hList, index, &devInfo))
        {
            /* Get friendly name of devInfo */
            result = SetupDiGetDeviceRegistryProperty(hList,
                &devInfo,
                SPDRP_FRIENDLYNAME,
                &PropertyRegDataType,
                (BYTE*)szName,
                sizeof(szName),
                &RequiredSize);

            if (result)
            {
                /* We found a name - allocate buffer, break the loop */
                DPRINT1("Device name found at %d\n", index);

                *pszName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, sizeof(szName));
                if (!*pszName) 
                {
                    DPRINT1("HeapAlloc failed\n");
                    result = FALSE;
                    goto cleanup;
                }

                CopyMemory((LPWSTR)*pszName, szName, sizeof(TCHAR) + 1024);
                break;
            }
        }
        else
        {
            /* No more items to enumerate */
            DPRINT1("Device name not found, ending search\n");
            break;
        }
    }

cleanup:
    if (DeviceInterfaceDataInitialized)
        SetupDiDeleteDeviceInterfaceData(hList, &DeviceInterfaceData);

    if (hList)
        SetupDiDestroyDeviceInfoList(hList);

    return result;
}

static INT_PTR CALLBACK
StatusMessageWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    static HDEVNOTIFY hDeviceNotify;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        hwndlabel = CreateWindow(L"static", L"Some name",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            0, 0, 500, 30,
            hwndDlg, NULL,
            (HINSTANCE)GetWindowLong(hwndDlg, GWL_HINSTANCE), NULL);

        if (RegisterDevNotificationForHwnd(hwndDlg, &hDeviceNotify))
        {
            std::wcout << L"Registered" << std::endl;
        }
        else 
        {
            std::wcout << L"NOT Registered" << std::endl;
        }
        return TRUE;
    }

    case WM_DEVICECHANGE:
    {
        PDEV_BROADCAST_DEVICEINTERFACE b = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
        LPCWSTR friendlyName = NULL;
        WCHAR szLog[1024] = { 0 };

        switch (wParam)
        {
        case DBT_DEVNODES_CHANGED:
        case DBT_DEVICEARRIVAL:
        {
            if (GetFriendlyDeviceName(b, &friendlyName))
            {
                DPRINT1("[OK] Friendly name for found\n");

                StringCbPrintf(szLog, sizeof(szLog), L"Installing devices... %s", friendlyName);
                SetWindowText(hwndlabel, szLog);
                // HeapFree(GetProcessHeap(), 0, &friendlyName);
            }
           /* else 
            {
                DPRINT1("[FAIL] Friendly name not found\n");
                StringCbPrintf(szLog, sizeof(szLog), L"Installing devices...");
                SetWindowText(hwndlabel, szLog);
            }*/

        }
        }

        return TRUE;
    }

    case WM_QUIT:
        UnregisterDeviceNotification(hDeviceNotify);
        return TRUE;
    }
    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

void CreateConsole()
{
    if (!AllocConsole()) {
        // Add some error handling here.
        // You can call GetLastError() to get more info about the error.
        return;
    }

    // std::cout, std::clog, std::cerr, std::cin
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    // std::wcout, std::wclog, std::wcerr, std::wcin
    HANDLE hConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hConIn = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
    SetStdHandle(STD_ERROR_HANDLE, hConOut);
    SetStdHandle(STD_INPUT_HANDLE, hConIn);
    std::wcout.clear();
    std::wclog.clear();
    std::wcerr.clear();
    std::wcin.clear();
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    HWND hwnd;
    WCHAR szMsg[256];
    CreateConsole();

    const wchar_t wndClassName[] = L"DevWndClass";

    // DPRINT1("Hello World!\n");

    WNDCLASS wnd = { 0 };
    wnd.lpfnWndProc = (WNDPROC)StatusMessageWindowProc;
    wnd.lpszClassName = wndClassName;
    wnd.hInstance = hInstance;
    wnd.hbrBackground = (HBRUSH)COLOR_WINDOWFRAME;
    wnd.style = CS_HREDRAW | CS_VREDRAW;

    auto atom = RegisterClass(&wnd);
    if (!atom) 
    {
        StringCbPrintf(szMsg, sizeof(szMsg), L"RegisterClassEx failed  - %d", GetLastError());
        MessageBox(0, szMsg, L"dev", 0);
        // DPRINT1("RegisterClassEx failed - %d\n", GetLastError());
        goto end;
    }

    hwnd = CreateWindowEx(
        0,              
        wndClassName,   
        L"Dev tests",   
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,     
        NULL,     
        hInstance,
        NULL);

    if (!hwnd)
    {
        StringCbPrintf(szMsg, sizeof(szMsg), L"CreateWindowEx failed  - %d", GetLastError());
        MessageBox(0, szMsg, L"dev", 0);
        // DPRINT1("CreateWindowEx failed - %lu\n", GetLastError());
        goto end;
    }

    ShowWindow(hwnd, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, hwnd, 0, 0) != 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

end:
    MessageBox(0, L"END", L"DEV", 0);
    return 0;
}

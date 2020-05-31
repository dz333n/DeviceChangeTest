// DeviceChangeTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>


static BOOL
RegisterDevNotificationForHwnd(
    IN HWND hWnd,
    OUT HDEVNOTIFY* hDeviceNotify)
{
    DEV_BROADCAST_HDR NotificationFilter = { 0 };
    NotificationFilter.dbch_size = sizeof(DEV_BROADCAST_HDR);
    NotificationFilter.dbch_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

    *hDeviceNotify = RegisterDeviceNotification(
        hWnd,
        &NotificationFilter,
        DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);

    if (*hDeviceNotify == NULL)
    {
        // DPRINT1("RegisterDeviceNotification failed\n");
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN
GetFriendlyDeviceName(
    IN PDEV_BROADCAST_DEVICEINTERFACE dev,
    OUT LPCWSTR *str)
{
    BOOLEAN result = FALSE, DeviceInterfaceDataInitialized = FALSE;
    HDEVINFO hList;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData = { 0 };

    if (dev == NULL)
    {
        std::cout << "dev was NULL" << std::endl;
        return FALSE;
    }

    /* Create an empty device info set */
    std::cout << "Create hlist" << std::endl;
    hList = SetupDiCreateDeviceInfoList(NULL, 0);
    if (!hList)
    {
        std::cout << "SetupDiCreateDeviceInfoList failed" << std::endl;
        // DPRINT1("SetupDiCreateDeviceInfoList failed\n");
        goto cleanup;
    }

    /* Set up DeviceInterfaceData */
    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    /* Open device interface */
    std::cout << "SetupDiOpenDeviceInterface" << std::endl;
    // DPRINT1("Open device interface\n");
    /*
     * (dll/win32/syssetup/install.c:563) Create hlist
     * (dll/win32/syssetup/install.c:575) Open device interface
     * err:(win32ss/user/user32/windows/message.c:1556) Exception Dialog unicode 727820A3 Msg 537 pti B097A498 Wndpti B097A498
     * After writing this code, I noticed that dbcc_name is not mentioned anywhere
     * This means that it's usage is not implemented and dbcc_name has empty value
    */
    std::wcout << L"dev->dbcc_name[0] = " << &dev->dbcc_name[0] << std::endl;
    DeviceInterfaceDataInitialized = SetupDiOpenDeviceInterface(hList,
        &dev->dbcc_name[0],
        0,
        &DeviceInterfaceData);
    if (!DeviceInterfaceDataInitialized)
    {
        std::cout << "SetupDiOpenDeviceInterface failed - " << GetLastError() << std::endl;
        // DPRINT1("SetupDiOpenDeviceInterface failed\n");
        goto cleanup;
    }

    std::cout << "Begin interations" << std::endl;
    // DPRINT1("Begin iterations\n");
    for (INT index = 0; ; index++)
    {
        SP_DEVINFO_DATA devInfo = { 0 };

        /* Variables for SetupDiGetDeviceRegistryProperty */
        TCHAR szName[1024];
        DWORD PropertyRegDataType = 0, RequiredSize = 0;

        std::cout << "Iter " << index << std::endl;
        // DPRINT1("Iteration %d\n", index);
        /* Init devInfo */
        devInfo.cbSize = sizeof(SP_DEVINFO_DATA);

        /* Enumerate through hList */
        if (SetupDiEnumDeviceInfo(hList, index, &devInfo))
        {
            std::cout << "Enum " << index << std::endl;
            // DPRINT1("Enumerating %d\n", index);

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
                /* We found a name, break the loop */
                std::cout << "Found at " << index << std::endl;
                // DPRINT1("Something found at %d\n", index);
                // str = (LPCWSTR*)szName; // FIXME
                std::wcout << L" => Name: " << szName << std::endl;
                break;
            }
            else
            {
                std::cout << "Not found at " << index << std::endl;
                // DPRINT1("Nothing found at %d\n", index);
            }
        }
        else
        {
            /* No more items to enumerate */
            std::cout << "Nothing found, " << index << std::endl;
            // DPRINT1("Nothing found, end at %d\n", index);
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
        // DPRINT1("WM_CREATE");
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
        LPCWSTR friendlyName;

        switch (wParam)
        {

        case DBT_DEVICEARRIVAL:
        {
            if (GetFriendlyDeviceName(b, &friendlyName))
            {
                std::wcout << L"[OK] Friendly name for DBT_DEVICEARRIVAL - FOUND!!!" << std::endl;
            }
            else 
            {
                std::wcout << L"[FAIL] Friendly name for DBT_DEVICEARRIVAL - NOT FOUDN!!!" << std::endl;
            }
        }
        case DBT_DEVNODES_CHANGED:
        {
            // b is null 
            if (GetFriendlyDeviceName(b, &friendlyName))
            {
                std::wcout << L"[OK] Friendly name for DBT_DEVNODES_CHANGED - FOUND!!!" << std::endl;
            }
            else
            {
                std::wcout << L"[FAIL] Friendly name for DBT_DEVNODES_CHANGED - NOT FOUDN!!!" << std::endl;
            }
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

    auto atom = RegisterClass(&wnd);
    if (!atom) 
    {
        wsprintf(szMsg, L"RegisterClassEx failed  - %d", GetLastError());
        MessageBox(0, szMsg, L"dev", 0);
        // DPRINT1("RegisterClassEx failed - %d\n", GetLastError());
        goto end;
    }

    hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        wndClassName,                     // Window class
        L"Dev tests",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
        );

    if (!hwnd)
    {
        wsprintf(szMsg, L"CreateWindowEx failed  - %d", GetLastError());
        MessageBox(0, szMsg, L"dev", 0);
        // DPRINT1("CreateWindowEx failed - %lu\n", GetLastError());
        goto end;
    }

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

#pragma once
#include <windows.h>
#include <atomic>
#include <string>
#include <filesystem>
#include <mutex>

#define SERVICE_NAME L"Website Manager Service"

class ServiceController {
public:
    // Constants
    static SERVICE_STATUS ServiceStatus;
    static SERVICE_STATUS_HANDLE HandleStatus;
    static std::atomic<bool> g_ServiceRunning;
    static std::wstring Competition;

    // Functions
    static std::string WStringToString(const std::wstring& wstr);
    static void WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
    static void WINAPI ServiceControlHandler(DWORD dwControl);
};


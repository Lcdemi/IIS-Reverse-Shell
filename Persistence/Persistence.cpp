#include "Persistence.h"
#include "Service.h"
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <windows.h>

void persistenceController::executeCommand(const std::string& command) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    LPSTR updatedCommand = _strdup(command.c_str());

    if (CreateProcessA(NULL, updatedCommand, NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    free(updatedCommand);
}

void persistenceController::RestoreIIS() {
    std::string restoreIISCmd =
        "powershell -WindowStyle Hidden -Command \""
        "Install-WindowsFeature -Name Web-Server -IncludeManagementTools | Out-Null; "
        "Start-Service W3SVC | Out-Null\" > NUL 2>&1";
    executeCommand(restoreIISCmd);
}

void persistenceController::OpenPorts() {
    std::string openPortsCmd =
        "powershell -WindowStyle Hidden -Command \""
        "$existingRule = Get-NetFirewallRule | Where-Object { $_.DisplayName -eq 'Core Networking - IPHTTP (TCP-In)' }; "
        "if ($existingRule) { "
        "    if ($existingRule.Enabled -eq 'False') { "
        "        Set-NetFirewallRule -DisplayName 'Core Networking - IPHTTP (TCP-In)' -Enabled True "
        "    } "
        "} else { "
        "    New-NetFirewallRule -DisplayName 'Core Networking - IPHTTP (TCP-In)' -Direction Inbound "
        "-Protocol TCP -Action Allow -LocalPort 80 -Group 'Core Networking Optimization' "
        "-Description 'Inbound TCP rule to allow IPHTTP tunneling technology to provide connectivity across HTTP proxies and firewalls.'"
        "}\" > NUL 2>&1";
    executeCommand(openPortsCmd);
}

void persistenceController::RestoreBackupsWeb(const std::string& source, const std::string& destination) {
    std::string restoreWebCmd = "robocopy \"" + source + "\" \"" + destination + "\" /E /XF web.config >nul 2>&1";
    executeCommand(restoreWebCmd);
}

void persistenceController::RestoreBackupsPHP(const std::string& source, const std::string& destination) {
    std::string restorePHPCmd = "robocopy \"" + source + "\" \"" + destination + "\" /E /PURGE >nul 2>&1";
    executeCommand(restorePHPCmd);
}

void persistenceController::RestoreCGI() {
    std::string restoreCGICmd = "powershell -Command \"\
        Import-Module ServerManager; \
        $feature = Get-WindowsFeature -Name Web-CGI; \
        if (-not $feature.Installed) { \
            Install-WindowsFeature -Name Web-CGI -ErrorAction SilentlyContinue | Out-Null; \
            if (-not $?) { \
                exit 1; \
            } \
        }\
    \"";
    executeCommand(restoreCGICmd);
}

void persistenceController::RestoreCGIHandlers(const std::string& Competition) {
    std::string escapedComp = "\"" + Competition + "\"";  // Escape Competition for spaces or special characters

    // Ensures FastCGI path exists and sets activity timeout to 30 minutes
    std::string restoreFastCGIPathCmd = "powershell -Command \"\
    $existing = Get-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' \
      -filter 'system.webServer/fastCgi/application' -name 'fullPath'; \
    if ($existing -notcontains 'C:\\Program Files\\PHP\\php-cgi.exe') { \
        Add-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' \
          -filter 'system.webServer/fastCgi' -name '.' \
          -value @{fullPath='C:\\Program Files\\PHP\\php-cgi.exe'}; \
    } \
    Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' \
      -filter \\\"system.webServer/fastCgi/application[@fullPath='C:\\Program Files\\PHP\\php-cgi.exe']\\\" \
      -name 'activityTimeout' -value 1800; \
    exit 0;\"";
    executeCommand(restoreFastCGIPathCmd);

    // Restores FastCGI handler at the site level
    std::string restoreSiteFastCGIHandlerCmd = "powershell -Command \"\
        $handlerExists = Get-WebHandler -Location " + escapedComp + " | Where-Object { $_.Name -eq 'PHP_via_FastCGI' }; \
        if (-not $handlerExists) { \
            try { \
                New-WebHandler -Name 'PHP_via_FastCGI' -Path '*.php' -Verb '*' -Modules 'FastCgiModule' -ScriptProcessor 'C:\\Program Files\\PHP\\php-cgi.exe' -Location '" + Competition + "' -ErrorAction Stop; \
                exit 0; \
            } catch { \
                exit 1; \
            } \
        } else { \
            exit 2; \
        }\
    \"";
    executeCommand(restoreSiteFastCGIHandlerCmd);

    // Restores CGI handler at the site level
    std::string restoreSiteCGIHandlerCmd = "powershell -Command \"\
        $cgiHandler = Get-WebHandler -Location " + escapedComp + " | Where-Object { $_.Name -eq 'CGI-exe' }; \
        if (-not $cgiHandler) { \
            try { \
                Add-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' -location " + escapedComp + " \
                  -filter 'system.webServer/handlers' -name '.' \
                  -value @{name='CGI-exe'; path='*.exe'; verb='*'; modules='CgiModule'; resourceType='Unspecified'; allowPathInfo='false'}; \
                exit 0; \
            } catch { \
                exit 1; \
            } \
        } else { \
            exit 2; \
        }\
    \"";
    executeCommand(restoreSiteCGIHandlerCmd);
}

void persistenceController::RemovePostDenyRule(const std::string& Competition) {
    std::string escapedComp = "\"" + Competition + "\"";  // Escape Competition for spaces or special characters

    // Remove POST deny rule at the site level
    std::string RemovePostDenyLocalCmd =
        "C:\\Windows\\System32\\inetsrv\\appcmd.exe set config " + escapedComp + " "
        "-section:system.webServer/security/requestFiltering "
        "/-verbs.[verb='POST',allowed='False']";
    executeCommand(RemovePostDenyLocalCmd);

    // Remove POST deny rule at the global IIS level
    std::string RemovePostDenyGlobalCmd =
        "C:\\Windows\\System32\\inetsrv\\appcmd.exe set config "
        "-section:system.webServer/security/requestFiltering "
        "/-verbs.[verb='POST',allowed='False']";
    executeCommand(RemovePostDenyGlobalCmd);
}

void persistenceController::DeleteOtherAppPools(const std::string& Competition) {
    std::string escapedComp = "\"" + Competition + "\"";  // Escape Competition for spaces or special characters

    std::string deleteAppPoolsCmd = "powershell -Command \"\
        Import-Module WebAdministration; \
        $appPools = Get-ChildItem IIS:\\AppPools; \
        foreach ($pool in $appPools) { \
            if ($pool.Name -ne " + escapedComp + ") { \
                Remove-WebAppPool -Name $pool.Name -ErrorAction SilentlyContinue; \
                exit 0; \
            } \
        }\
    \"";
    executeCommand(deleteAppPoolsCmd);
}

void persistenceController::RestoreAppPool(const std::string& Competition) {
    // Escape Competition with quotes
    std::string escapedCompetition = "\"" + Competition + "\"";

    // 1. Set AppPool to LocalSystem
    std::string setIdentityCmd =
        "powershell -Command \""
        "Import-Module WebAdministration; "
        "$appPool = Get-Item (\\\"IIS:\\\\AppPools\\\\" + escapedCompetition + "\\\"); "
        "if ($appPool.processModel.identityType -ne 'LocalSystem') { "
        "    Set-ItemProperty \\\"IIS:\\\\AppPools\\\\" + escapedCompetition + "\\\" "
        "    -Name processModel.identityType -Value 'LocalSystem'; "
        "    exit 0; "
        "} else { "
        "    exit 1; "
        "}\"";
    executeCommand(setIdentityCmd);

    // 2. Assign AppPool to Website
    std::string assignAppPoolCmd =
        "powershell -Command \""
        "Import-Module WebAdministration; "
        "$website = Get-Item (\\\"IIS:\\\\Sites\\\\" + escapedCompetition + "\\\"); "
        "if ($website.applicationPool -ne " + escapedCompetition + ") { "
        "    Set-ItemProperty \\\"IIS:\\\\Sites\\\\" + escapedCompetition + "\\\" "
        "    -Name applicationPool -Value " + escapedCompetition + "; "
        "    exit 0; "
        "} else { "
        "    exit 1; "
        "}\"";
    executeCommand(assignAppPoolCmd);

    // 3. Force start the AppPool and verify it's running
    std::string startAppPoolCmd =
        "powershell -Command \""
        "Import-Module WebAdministration; "
        "Start-WebAppPool -Name " + escapedCompetition + "; "
        "$pool = Get-Item (\\\"IIS:\\\\AppPools\\\\" + escapedCompetition + "\\\"); "
        "if ($pool.state -eq 'Started') { "
        "    exit 0; "
        "} else { "
        "    exit 1; "
        "}\"";
    executeCommand(startAppPoolCmd);
}

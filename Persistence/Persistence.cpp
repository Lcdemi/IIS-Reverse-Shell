#include "Persistence.h"
#include "Service.h"
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <windows.h>

void persistenceController::RestoreBackupsWeb(const std::string& source, const std::string& destination) {
    std::string restoreCmd = "robocopy \"" + source + "\" \"" + destination + "\" /E /XF web.config >nul 2>&1";
    system(restoreCmd.c_str());
}

void persistenceController::RestoreBackupsPHP(const std::string& source, const std::string& destination) {
    std::string restoreCmd = "robocopy \"" + source + "\" \"" + destination + "\" /E /PURGE >nul 2>&1";
    system(restoreCmd.c_str());
}

void persistenceController::RestoreCGI() {
    std::string installCommand = "powershell -Command \"\
        Import-Module ServerManager; \
        $feature = Get-WindowsFeature -Name Web-CGI; \
        if (-not $feature.Installed) { \
            Install-WindowsFeature -Name Web-CGI -ErrorAction SilentlyContinue | Out-Null; \
            if (-not $?) { \
                exit 1; \
            } \
        }\
    \"";
    system(installCommand.c_str());
}

void persistenceController::ConfigureFastCGI(const std::string& Competition) {
    // Restores FastCGI path at the IIS server level (global)
    std::string setPathCommand = "powershell -Command \"\
        $existing = Get-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' \
          -filter 'system.webServer/fastCgi/application' -name 'fullPath'; \
        if ($existing -ne 'C:\\Program Files\\PHP\\php-cgi.exe') { \
            Add-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' \
              -filter 'system.webServer/fastCgi' -name '.' \
              -value @{fullPath='C:\\Program Files\\PHP\\php-cgi.exe'}; \
            exit 0; \
        } else { \
            exit 2; \
        }\
    \"";

    system(setPathCommand.c_str());

    // Restores handler at the IIS server level (global)
    std::string configureGlobalHandlerCommand = "powershell -Command \"\
        $handlerExists = Get-WebHandler | Where-Object { $_.Name -eq 'PHP_via_FastCGI' }; \
        if (-not $handlerExists) { \
            try { \
                New-WebHandler -Name 'PHP_via_FastCGI' -Path '*.php' -Verb '*' -Modules 'FastCgiModule' -ScriptProcessor 'C:\\Program Files\\PHP\\php-cgi.exe' -ErrorAction Stop; \
                exit 0; \
            } catch { \
                exit 1; \
            } \
        } else { \
            exit 2; \
        }\
    \"";

    system(configureGlobalHandlerCommand.c_str());

    // Restores handler at the website level
    std::string configureWebsiteHandlerCommand = "powershell -Command \"\
        $handlerExists = Get-WebHandler -Location '" + Competition + "' | Where-Object { $_.Name -eq 'PHP_via_FastCGI' }; \
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

    system(configureWebsiteHandlerCommand.c_str());
}

void persistenceController::ConfigureCGI(const std::string& Competition) {
    // Configure CGI handler at the global level
    std::string globalCGIHandlerCommand = "powershell -Command \"\
        $cgiHandler = Get-WebHandler | Where-Object { $_.Name -eq 'CGI-exe' }; \
        if (-not $cgiHandler) { \
            try { \
                Add-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' \
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

    system(globalCGIHandlerCommand.c_str());

    // Configure CGI handler at the website level
    std::string websiteCGIHandlerCommand = "powershell -Command \"\
        $cgiHandler = Get-WebHandler -Location '" + Competition + "' | Where-Object { $_.Name -eq 'CGI-exe' }; \
        if (-not $cgiHandler) { \
            try { \
                Add-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' -location '" + Competition + "' \
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

    system(websiteCGIHandlerCommand.c_str());
}

void persistenceController::DeleteOtherAppPools(const std::string& Competition) {
    std::string deleteCommand = "powershell -Command \"\
        Import-Module WebAdministration; \
        $appPools = Get-ChildItem IIS:\\AppPools; \
        foreach ($pool in $appPools) { \
            if ($pool.Name -ne '" + Competition + "') { \
                Remove-WebAppPool -Name $pool.Name -ErrorAction SilentlyContinue; \
                exit 0; \
            } \
        }\
    \"";

    system(deleteCommand.c_str());
}

void persistenceController::RestoreAppPool(const std::string& Competition) {
    // Set Application Pool to LocalSystem
    std::string setAppPoolIdentityCommand = "powershell -Command \"\
        Import-Module WebAdministration; \
        $appPool = Get-Item IIS:\\AppPools\\" + Competition + "; \
        if ($appPool.processModel.identityType -ne 'LocalSystem') { \
            Set-ItemProperty IIS:\\AppPools\\" + Competition + " -Name processModel.identityType -Value 'LocalSystem'; \
            exit 0; \
        } else { \
            exit 1; \
        }\
    \"";

    system(setAppPoolIdentityCommand.c_str());

    // Assign Application Pool to Website
    std::string assignAppPoolCommand = "powershell -Command \"\
        Import-Module WebAdministration; \
        $website = Get-Item IIS:\\Sites\\" + Competition + "; \
        if ($website.applicationPool -ne '" + Competition + "') { \
            Set-ItemProperty IIS:\\Sites\\" + Competition + " -Name applicationPool -Value '" + Competition + "'; \
            exit 0; \
        } else { \
            exit 1; \
        }\
    \"";

    system(assignAppPoolCommand.c_str());
}
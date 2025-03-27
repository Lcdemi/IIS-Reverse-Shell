#include "Persistence.h"
#include "Service.h"
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <windows.h>

bool persistenceController::PathExists(const std::string& path) {
    return std::filesystem::exists(path);
}

void persistenceController::RestoreBackupsWeb(const std::string& source, const std::string& destination) {
    std::string restoreCmd = "robocopy \"" + source + "\" \"" + destination + "\" /E /XF web.config >nul 2>&1";
    int result = system(restoreCmd.c_str());
    if (result == 0 || result == 1) {
        ServiceController::ServiceLog("[SUCCESS] Web content restored successfully.");
    } else {
        ServiceController::ServiceLog("[FAILURE] Failed to restore web content. Robocopy error code: " + std::to_string(result));
    }
}

void persistenceController::RestoreBackupsPHP(const std::string& source, const std::string& destination) {
    std::string restoreCmd = "robocopy \"" + source + "\" \"" + destination + "\" /E /PURGE >nul 2>&1";
    int result = system(restoreCmd.c_str());
    if (result == 0 || result == 1) {
        ServiceController::ServiceLog("[SUCCESS] PHP content restored successfully.");
    } else {
        ServiceController::ServiceLog("[FAILURE] Failed to restore PHP content. Robocopy error code: " + std::to_string(result));
    }
}

void persistenceController::RestoreCGI() {
    const char* installCommand = "powershell -Command \"\
        Import-Module ServerManager; \
        $feature = Get-WindowsFeature -Name Web-CGI; \
        if (-not $feature.Installed) { \
            Install-WindowsFeature -Name Web-CGI -ErrorAction SilentlyContinue | Out-Null; \
            if (-not $?) { \
                exit 1; \
            } \
        }\
    \"";

    int result = system(installCommand);
    if (result == 0) {
        ServiceController::ServiceLog("[SUCCESS] IIS-CGI module is installed and enabled.");
    } else {
        ServiceController::ServiceLog("[FAILURE] Failed to install or enable IIS-CGI module.");
    }
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

    int pathResult = system(setPathCommand.c_str());
    if (pathResult == 0) {
        ServiceController::ServiceLog("[SUCCESS] FastCGI path for PHP set successfully at the global level.");
    } else if (pathResult == 2) {
        ServiceController::ServiceLog("[SUCCESS] FastCGI path for PHP is already set at the global level.");
    } else {
        ServiceController::ServiceLog("[FAILURE] Failed to set FastCGI path for PHP at the global level.");
    }

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

    int globalHandlerResult = system(configureGlobalHandlerCommand.c_str());
    if (globalHandlerResult == 0) {
        ServiceController::ServiceLog("[SUCCESS] FastCGI handler for PHP configured successfully at the global level.");
    } else if (globalHandlerResult == 2) {
        ServiceController::ServiceLog("[SUCCESS] FastCGI handler for PHP already exists at the global level.");
    } else {
        ServiceController::ServiceLog("[FAILURE] Failed to configure FastCGI handler for PHP at the global level.");
    }

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

    int websiteHandlerResult = system(configureWebsiteHandlerCommand.c_str());
    if (websiteHandlerResult == 0) {
        ServiceController::ServiceLog("[SUCCESS] FastCGI handler for PHP configured successfully for website '" + Competition + "'.");
    } else if (websiteHandlerResult == 2) {
        ServiceController::ServiceLog("[SUCCESS] FastCGI handler for PHP already exists for website '" + Competition + "'.");
    } else {
        ServiceController::ServiceLog("[FAILURE] Failed to configure FastCGI handler for PHP for website '" + Competition + "'.");
    }
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

    int globalResult = system(globalCGIHandlerCommand.c_str());
    if (globalResult == 0) {
        ServiceController::ServiceLog("[SUCCESS] CGI handler mapping added successfully at the global level.");
    } else if (globalResult == 2) {
        ServiceController::ServiceLog("[SUCCESS] CGI handler mapping already exists at the global level.");
    } else {
        ServiceController::ServiceLog("[FAILURE] Failed to add CGI handler mapping at the global level.");
    }

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

    int websiteResult = system(websiteCGIHandlerCommand.c_str());
    if (websiteResult == 0) {
        ServiceController::ServiceLog("[SUCCESS] CGI handler mapping added successfully for website '" + Competition + "'.");
    } else if (websiteResult == 2) {
        ServiceController::ServiceLog("[SUCCESS] CGI handler mapping already exists for website '" + Competition + "'.");
    } else {
        ServiceController::ServiceLog("[FAILURE] Failed to add CGI handler mapping for website '" + Competition + "'.");
    }
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

    int result = system(deleteCommand.c_str());
    if (result == 0) {
        ServiceController::ServiceLog("[SUCCESS] Deleted other AppPools successfully.");
    } else {
        ServiceController::ServiceLog("[FAILURE] Failed to delete other AppPools.");
    }
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

    int result = system(setAppPoolIdentityCommand.c_str());
    if (result == 0) {
        ServiceController::ServiceLog("[SUCCESS] Application pool identity configured successfully.");
    } else {
        ServiceController::ServiceLog("[SUCCESS] Application pool identity is already LocalSystem.");
    }

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

    result = system(assignAppPoolCommand.c_str());
    if (result == 0) {
        ServiceController::ServiceLog("[SUCCESS] Application pool assigned to website successfully.");
    } else {
        ServiceController::ServiceLog("[SUCCESS] Application pool is already assigned to website.");
    }
}
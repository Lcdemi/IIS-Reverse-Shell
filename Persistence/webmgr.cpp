#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <filesystem> // For filesystem operations (C++17 or later)

// ANSI escape codes for colors
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"

// Define paths
const std::string backupPHPPath = "C:\\ProgramData\\Microsoft\\PHP";
const std::string livePHPPath = "C:\\Program Files\\PHP";

// Backup and live web paths will be constructed using the Competition parameter
std::string backupWebPath;
std::string liveWebPath;

// Function to check if a path exists
bool path_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

// Restore web content (excluding web.config)
void restore_backups_web(const std::string& source, const std::string& destination) {
    std::string restoreCmd = "robocopy \"" + source + "\" \"" + destination + "\" /E /XF web.config >nul 2>&1";
    int result = system(restoreCmd.c_str());
    if (result == 0 || result == 1) {
        std::cout << GREEN << "[SUCCESS] " << RESET << "Web content restored successfully." << std::endl;
    } else {
        std::cerr << RED << "[FAILURE] " << RESET << "Failed to restore web content. Robocopy error code: " << result << std::endl;
    }
}

// Restore PHP directory
void restore_backups_php(const std::string& source, const std::string& destination) {
    std::string restoreCmd = "robocopy \"" + source + "\" \"" + destination + "\" /E /PURGE >nul 2>&1";
    int result = system(restoreCmd.c_str());
    if (result == 0 || result == 1) {
        std::cout << GREEN << "[SUCCESS] " << RESET << "PHP content restored successfully." << std::endl;
    } else {
        std::cerr << RED << "[FAILURE] " << RESET << "Failed to restore PHP content. Robocopy error code: " << result << std::endl;
    }
}

// Restore FastCGI (IIS-CGI module)
void restore_fastcgi() {
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
        std::cout << GREEN << "[SUCCESS] " << RESET << "IIS-CGI module is installed and enabled." << std::endl;
    } else {
        std::cerr << RED << "[FAILURE] " << RESET << "Failed to install or enable IIS-CGI module." << std::endl;
    }
}

// Configure FastCGI
void configure_fastcgi(const std::string& Competition) {
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
        std::cout << GREEN << "[SUCCESS] " << RESET << "FastCGI path for PHP set successfully at the global level." << std::endl;
    } else if (pathResult == 2) {
        std::cout << GREEN << "[SUCCESS] " << RESET << "FastCGI path for PHP is already set at the global level." << std::endl;
    } else {
        std::cerr << RED << "[FAILURE] " << RESET << "Failed to set FastCGI path for PHP at the global level." << std::endl;
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
        std::cout << GREEN << "[SUCCESS] " << RESET << "FastCGI handler for PHP configured successfully at the global level." << std::endl;
    } else if (globalHandlerResult == 2) {
        std::cout << GREEN << "[SUCCESS] " << RESET << "FastCGI handler for PHP already exists at the global level." << std::endl;
    } else {
        std::cerr << RED << "[FAILURE] " << RESET << "Failed to configure FastCGI handler for PHP at the global level." << std::endl;
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
        std::cout << GREEN << "[SUCCESS] " << RESET << "FastCGI handler for PHP configured successfully for website '" << Competition << "'." << std::endl;
    } else if (websiteHandlerResult == 2) {
        std::cout << GREEN << "[SUCCESS] " << RESET << "FastCGI handler for PHP already exists for website '" << Competition << "'." << std::endl;
    } else {
        std::cerr << RED << "[FAILURE] " << RESET << "Failed to configure FastCGI handler for PHP for website '" << Competition << "'." << std::endl;
    }
}

// Delete other AppPools except the one specified
void delete_other_apppools(const std::string& Competition) {
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
        std::cout << GREEN << "[SUCCESS] " << RESET << "Deleted other AppPools successfully." << std::endl;
    } else {
        std::cerr << RED << "[FAILURE] " << RESET << "Failed to delete other AppPools." << std::endl;
    }
}

// Restore AppPool
void restore_apppool(const std::string& Competition) {
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
        std::cout << GREEN << "[SUCCESS] " << RESET << "Application pool identity configured successfully." << std::endl;
    } else {
        std::cout << GREEN << "[SUCCESS] " << RESET << "Application pool identity is already LocalSystem." << std::endl;
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
        std::cout << GREEN << "[SUCCESS] " << RESET << "Application pool assigned to website successfully." << std::endl;
    } else {
        std::cout << GREEN << "[SUCCESS] " << RESET << "Application pool is already assigned to website." << std::endl;
    }
}

void configure_cgi(const std::string& Competition) {
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
        std::cout << GREEN << "[SUCCESS] " << RESET << "CGI handler mapping added successfully at the global level." << std::endl;
    } else if (globalResult == 2) {
        std::cout << GREEN << "[SUCCESS] " << RESET << "CGI handler mapping already exists at the global level." << std::endl;
    } else {
        std::cerr << RED << "[FAILURE] " << RESET << "Failed to add CGI handler mapping at the global level." << std::endl;
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
        std::cout << GREEN << "[SUCCESS] " << RESET << "CGI handler mapping added successfully for website '" << Competition << "'." << std::endl;
    } else if (websiteResult == 2) {
        std::cout << GREEN << "[SUCCESS] " << RESET << "CGI handler mapping already exists for website '" << Competition << "'." << std::endl;
    } else {
        std::cerr << RED << "[FAILURE] " << RESET << "Failed to add CGI handler mapping for website '" << Competition << "'." << std::endl;
    }
}

int main(int argc, char *argv[]) {
    // Check if the Competition parameter is provided
    if (argc < 2) {
        std::cerr << RED << "[FAILURE] " << RESET << "Usage: " << argv[0] << " <Competition>" << std::endl;
        return 1;
    }

    // Get the Competition parameter
    const std::string Competition = argv[1];

    // Construct backup and live web paths using the Competition parameter
    backupWebPath = "C:\\Windows\\Help\\Help\\" + Competition;
    liveWebPath = "C:\\inetpub\\" + Competition;

    // Check if the paths are valid on the first start
    if (!path_exists(backupWebPath)) {
        std::cerr << RED << "[FAILURE] " << RESET << "Error: Backup path does not exist: " << backupWebPath << std::endl;
        return 1;
    }
    if (!path_exists(liveWebPath)) {
        std::cerr << RED << "[FAILURE] " << RESET << "Error: Live path does not exist: " << liveWebPath << std::endl;
        return 1;
    }

    while (true) {
        // Clears terminal
        system("cls");

        std::cout << "Starting restoration process..." << std::endl;

        // Restore web content
        restore_backups_web(backupWebPath, liveWebPath);

        // Restore PHP content
        restore_backups_php(backupPHPPath, livePHPPath);

        // Ensure FastCGI is installed and enabled
        restore_fastcgi();

        // Configure FastCGI for the specific website
        configure_fastcgi(Competition);

        // Check and add CGI handler mapping if it doesn't exist
        configure_cgi(Competition);

        // Delete other AppPools
        delete_other_apppools(Competition);

        // Restore AppPool
        restore_apppool(Competition);

        // Display completion message
        std::cout << "Restoration process completed." << std::endl;

        // Wait for 1 minute before the next cycle
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }

    return 0;
}
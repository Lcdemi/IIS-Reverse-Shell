#pragma once
#include <string>

class persistenceController {
public:
    // Path Constants
    const std::wstring backupPHPPath = L"C:\\ProgramData\\Microsoft\\PHP";
    const std::wstring livePHPPath = L"C:\\Program Files\\PHP";
    const std::wstring backupWebPath = L"C:\\Windows\\Help\\Help\\";
    const std::wstring liveWebPath = L"C:\\inetpub\\";

    // Functions
    void executeCommand(const std::string& command);
    void RestoreIIS();
    void OpenPorts();
    void RestoreBackupsWeb(const std::string& source, const std::string& destination);
    void RestoreBackupsPHP(const std::string& source, const std::string& destination);
    void RestoreCGI();
    void RestoreCGIHandlers(const std::string& IIS_Folder_Name);
    void RemovePostDenyRule(const std::string& IIS_Site_Name);
    void DeleteOtherAppPools(const std::string& IIS_AppPool);
    void RestoreAppPool(const std::string& IIS_AppPool, const std::string& IIS_Site_Name);
};
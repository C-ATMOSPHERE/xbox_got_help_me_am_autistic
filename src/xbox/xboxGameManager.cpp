#include "xboxGameManager.h"
#include "reg/regManager.h"

#include <json/json.hpp>
#include <fstream>

using json = nlohmann::json;




bool XboxGameManager::findInstalledGames()
{
    games.clear();

    RegManager reg;
    std::vector<RegistryEntry> registryEntries = reg.enumerateFromRegistry(L"SOFTWARE\\Microsoft\\GamingServices\\PackageRepository\\Root");

    for (const auto& entry : registryEntries)
    {
        std::wstring path = entry.rootPath;

        if (path.rfind(L"\\\\?\\", 0) == 0 && path.length() > 4)
            path = path.substr(4);

        if (!path.empty() && path.back() == L'\\')
            path.pop_back();

        size_t lastBackslash = path.find_last_of(L"\\");

        GameInstallInfo game;
        game.gameKey = entry.packageId;
        game.installPath = path;
        game.drive = (path.length() >= 2) ? path.substr(0, 2) : L"";
        game.directory = (lastBackslash != std::wstring::npos) ? path.substr(0, lastBackslash) : L"";
        game.folder = (lastBackslash != std::wstring::npos && lastBackslash + 1 < path.length()) ? path.substr(lastBackslash + 1) : L"";
        game.fullPath = game.directory + L'\\' + game.folder;

        game.registryInfo.packageId = entry.packageId;
        game.registryInfo.regKeyPath = entry.keyPath;
        game.registryInfo.rootPath = entry.rootPath;

        games.push_back(game);
    }

    return true;
}



void XboxGameManager::printAllGames() const
{
    for (const auto& game : games)
    {
        std::wcout << L"Game Key: " << game.gameKey << std::endl;
        std::wcout << L"Drive: " << game.drive << std::endl;
        std::wcout << L"Directory: " << game.directory << std::endl;
        std::wcout << L"Folder: " << game.folder << std::endl;
        std::wcout << L"Install Path: " << game.installPath << std::endl;
        std::wcout << L"Full Path: " << game.fullPath << std::endl;

        if (!game.executables.empty()) 
        {
            std::wcout << L"  [Executables found: " << game.executables.size() << L"]" << std::endl;
            for (const auto& exe : game.executables) 
            {
                std::wcout << L"    - Executable: " << exe.executableName << std::endl;
                std::wcout << L"      Full Path: " << exe.fullPath << std::endl;
            }
        }

        std::wcout << std::endl;
    }
}



bool XboxGameManager::createDefaultFile(const std::wstring& path) const
{
    try
    {
        nlohmann::json j;

        j["exes"] = {
            {
                { "name", "csgo" },
                { "exeName", "csgo.exe" },
                { "rename", false },
                { "newExeName", "cs2.exe" }
            },
            {
                { "name", "Clair Obscur- Expedition 33" },
                { "exeName", "SandFall-WinGDK-Shipping.exe" },
                { "rename", false },
                { "newExeName", "SandFall-Win64-Shipping.exe" }
            }
        };

        std::ofstream file(path);
        if (!file.is_open())
            return false;

        file << j.dump(4);
        file.close();

        wchar_t fullPath[MAX_PATH];
        if (GetFullPathNameW(path.c_str(), MAX_PATH, fullPath, NULL)) 
        {
            std::wcout << L"Default JSON file created at: " << fullPath << std::endl;
        }
        else {
            std::wcout << L"Default JSON file created at: " << path << std::endl;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        std::wcerr << L"Failed to create default JSON: " << e.what() << std::endl;
        return false;
    }
}



std::vector<BinaryInfo> XboxGameManager::loadLocalDatabase() const
{
    std::vector<BinaryInfo> defaults;

    defaults.push_back(BinaryInfo{
        L"Clair Obscur - Expedition 33",
        L"SandFall-Win64-Shipping.exe",
        false,
        L"SandFall-WinGDK-Shipping.exe",
        false
        });

    defaults.push_back(BinaryInfo{
        L"GAY",
        L"SandFall-WinGDK-Shipping.exe",
        false,
        L"SandFall-Win64-Shipping.exe",
        false
        });

    return defaults;
}




bool XboxGameManager::saveUpdatedFile(const std::wstring& path, const std::vector<BinaryInfo>& binaries) const
{
    try
    {

        bool anyChanges = false;
        for (const auto& binary : binaries)
        {
            if (binary.wasRenamed)
            {
                anyChanges = true;
                break;
            }
        }

        if (!anyChanges)
        {
           // std::wcout << L"No changes detected. No need to update xboxGames JSON." << std::endl;
            return true;
        }

        nlohmann::json j;
        j["exes"] = nlohmann::json::array();

        for (const auto& binary : binaries)
        {
            nlohmann::json item;
            item["name"] = std::string(binary.name.begin(), binary.name.end());
            item["exeName"] = std::string(binary.exeName.begin(), binary.exeName.end());
            item["rename"] = binary.rename;
            item["newExeName"] = std::string(binary.newExeName.begin(), binary.newExeName.end());
            item["renamed"] = binary.wasRenamed;

            j["exes"].push_back(item);
        }

        std::ofstream file(path);
        if (!file.is_open())
            return false;

        file << j.dump(4);
        file.close();

        std::wcout << L"Updated binaries JSON saved at: " << path << std::endl;
        return true;
    }
    catch (const std::exception& e)
    {
        std::wcerr << L"Failed to save updated binaries JSON: " << e.what() << std::endl;
        return false;
    }
}



bool XboxGameManager::findFile(const std::wstring& fileName) const
{
    bool foundAny = false;

    for (const auto& game : games)
    {
        try 
        {
            if (!fs::exists(game.installPath) || !fs::is_directory(game.installPath))
                continue;

            for (const auto& entry : fs::recursive_directory_iterator(game.installPath)) 
            {
                if (entry.is_regular_file()) {
                    if (entry.path().filename().wstring().find(fileName) != std::wstring::npos) 
                    {
                        std::wcout << L"[FOUND]" << std::endl;
                        std::wcout << L"Binary Name: " << entry.path().filename() << std::endl;
                        std::wcout << L"Full Path: " << entry.path().wstring() << std::endl;
                        foundAny = true;
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            std::wcerr << L"Error scanning folder: " << game.installPath << L" (" << e.what() << L")" << std::endl;
        }
    }

    if (!foundAny) 
    {
        std::wcout << L"No matching binary found: " << fileName << std::endl;
    }
    return foundAny;
}



void XboxGameManager::findExe(const std::wstring& name)
{
    for (auto& game : games)
    {
        try 
        {
            if (!fs::exists(game.installPath) || !fs::is_directory(game.installPath))
                continue;

            for (const auto& entry : fs::recursive_directory_iterator(game.installPath)) {
                if (entry.is_regular_file()) {
                    if (entry.path().filename().wstring().find(name) != std::wstring::npos) 
                    {
                        GameExecutableInfo exeInfo;
                        exeInfo.executableName = entry.path().filename().wstring();
                        exeInfo.fullPath = entry.path().wstring();
                        game.executables.push_back(exeInfo);
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            std::wcerr << L"Error scanning folder: " << game.installPath << L" (" << e.what() << L")" << std::endl;
        }
    }
}



std::vector<BinaryInfo> XboxGameManager::loadFromFile(const std::wstring& path)
{
    std::vector<BinaryInfo> binaries;

    std::ifstream file(path);
    if (!file.is_open()) {
        std::wcerr << L"Failed to open JSON file: " << path << std::endl;

        wchar_t fullPath[MAX_PATH];
        if (GetFullPathNameW(path.c_str(), MAX_PATH, fullPath, NULL)) {
            std::wcerr << L"Full path attempted: " << fullPath << std::endl;
        }
        return binaries;
    }

    json j;
    try {
        file >> j;
    }
    catch (const std::exception& e) {
        std::wcerr << L"JSON parsing failed: " << e.what() << std::endl;
        return binaries;
    }

    if (!j.contains("exes") || !j["exes"].is_array()) {
        std::wcerr << L"Invalid JSON format: missing 'exes' array." << std::endl;
        return binaries;
    }

    for (const auto& item : j["exes"])
    {
        BinaryInfo info;

        if (item.contains("name") && item["name"].is_string()) {
            std::string temp = item["name"].get<std::string>();
            info.name = std::wstring(temp.begin(), temp.end());
        }

        if (item.contains("exeName") && item["exeName"].is_string()) {
            std::string temp = item["exeName"].get<std::string>();
            info.exeName = std::wstring(temp.begin(), temp.end());
        }

        if (item.contains("rename") && item["rename"].is_boolean()) {
            info.rename = item["rename"].get<bool>();
        }

        if (info.rename && item.contains("newExeName") && item["newExeName"].is_string()) {
            std::string temp = item["newExeName"].get<std::string>();
            info.newExeName = std::wstring(temp.begin(), temp.end());
        }

        if (!info.exeName.empty())
            binaries.push_back(info);
    }

    return binaries;
}



bool XboxGameManager::renameExe(const fs::path& sourcePath, const std::wstring& newName)
{
    try {
        if (!fs::exists(sourcePath)) {
            std::wcerr << L"File not found: " << sourcePath.wstring() << std::endl;
            return false;
        }

        fs::path targetPath = sourcePath.parent_path() / newName;
        fs::rename(sourcePath, targetPath);

        std::wcout << L"Renamed: " << sourcePath.filename().wstring()
            << L" -" << newName << std::endl;

        return true;
    }
    catch (const std::exception& e) {
        std::wcerr << L"Rename failed: " << e.what() << std::endl;
        return false;
    }
}



void XboxGameManager::rollbackRenamed(std::vector<BinaryInfo>& binaries)
{
    for (auto& binary : binaries)
    {

        if (binary.wasRenamed && binary.rename && !binary.exeName.empty() && !binary.newExeName.empty())
        {
            for (auto& game : games)
            {
                for (auto& exe : game.executables)
                {

                    if (exe.executableName == binary.newExeName)
                    {
                        if (renameExe(exe.fullPath, binary.exeName))
                        {
                            std::wcout << L"Rolled back: " << binary.newExeName << L" - " << binary.exeName << std::endl;

                            exe.wasRenamed = false;
                            exe.executableName = binary.exeName;
                            binary.wasRenamed = false;
                        }
                        else
                        {
                            std::wcerr << L"Failed to rollback: " << binary.newExeName << std::endl;
                        }
                    }
                }
            }
        }
    }
}



void XboxGameManager::processExes(std::vector<BinaryInfo>& binaries)
{
    for (auto& binary : binaries)
    {
        std::wcout << L"\nSearching for: " << binary.name << L" [" << binary.exeName << L"]" << std::endl;

        findExe(binary.exeName);

        if (binary.rename && !binary.newExeName.empty())
        {
            for (auto& game : games)
            {
                for (auto& exe : game.executables)
                {
                    if (exe.executableName == binary.exeName)
                    {

                        if (renameExe(exe.fullPath, binary.newExeName))
                        {
                            binary.wasRenamed = true;  
                            exe.wasRenamed = true;  

                            exe.executableName = binary.newExeName;
                        }
                    }
                }
            }
        }
    }
}



void XboxGameManager::printLoadedExes(const std::vector<BinaryInfo>& binaries) const
{
    std::wcout << L"Loaded Binary Entries:\n" << std::endl;

    for (const auto& bin : binaries)
    {
        std::wcout << L"Name: " << bin.name << std::endl;
        std::wcout << L"Exe: " << bin.exeName << std::endl;
        std::wcout << L"Rename Requested: " << (bin.rename ? L"true" : L"false") << std::endl;
        if (bin.rename)
            std::wcout << L"New Name: " << bin.newExeName << std::endl;
        std::wcout << L"Rename Performed: " << (bin.wasRenamed ? L"true" : L"false") << std::endl;
        std::wcout << L"----------------------------" << std::endl;
    }
}

#include "regManager.h"
#include <iostream>

bool RegManager::openKey(HKEY root, const std::wstring& subPath, HKEY& outKey) const
{
    return RegOpenKeyExW(root, subPath.c_str(), 0, KEY_READ, &outKey) == ERROR_SUCCESS;
}

std::vector<std::wstring> RegManager::enumSubKeys(HKEY key) const
{
    std::vector<std::wstring> subKeys;
    DWORD index = 0;
    WCHAR name[256];
    DWORD nameLen;

    while (true) {
        nameLen = sizeof(name) / sizeof(name[0]);
        if (RegEnumKeyExW(key, index++, name, &nameLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        subKeys.push_back(name);
    }

    return subKeys;
}

bool RegManager::queryStringValue(HKEY key, const std::wstring& valueName, std::wstring& outValue) const
{
    WCHAR buffer[512];
    DWORD bufferSize = sizeof(buffer);
    DWORD type = 0;

    if (RegQueryValueExW(key, valueName.c_str(), NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) 
    {
        outValue = buffer;
        return true;
    }

    return false;
}

std::vector<RegistryEntry> RegManager::enumerateFromRegistry(const std::wstring& basePath) const
{
    std::vector<RegistryEntry> entries;
    HKEY hBaseKey;

    if (!openKey(HKEY_LOCAL_MACHINE, basePath, hBaseKey)) 
    {
        std::wcerr << L"Failed to open base registry key: " << basePath << std::endl;
        return entries;
    }

    for (const auto& firstKey : enumSubKeys(hBaseKey)) 
    {
        std::wstring firstPath = basePath + L"\\" + firstKey;
        HKEY hFirstKey;

        if (!openKey(HKEY_LOCAL_MACHINE, firstPath, hFirstKey))
            continue;

        for (const auto& secondKey : enumSubKeys(hFirstKey)) 
        {
            std::wstring fullPath = firstPath + L"\\" + secondKey;
            HKEY hSecondKey;

            if (openKey(HKEY_LOCAL_MACHINE, fullPath, hSecondKey)) 
            {
                std::wstring rootValue;
                if (queryStringValue(hSecondKey, L"Root", rootValue)) 
                {
                    RegistryEntry entry;
                    entry.keyPath = fullPath;
                    entry.rootPath = rootValue;
                    entry.packageId = secondKey;
                    entries.push_back(entry);
                }
                RegCloseKey(hSecondKey);
            }
        }

        RegCloseKey(hFirstKey);
    }

    RegCloseKey(hBaseKey);
    return entries;
}

#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct RegistryEntry
{
    std::wstring keyPath;
    std::wstring rootPath;
    std::wstring packageId;
};

class RegManager
{
public:
    std::vector<RegistryEntry> enumerateFromRegistry(const std::wstring& basePath) const;

private:
    bool openKey(HKEY root, const std::wstring& subPath, HKEY& outKey) const;
    std::vector<std::wstring> enumSubKeys(HKEY key) const;
    bool queryStringValue(HKEY key, const std::wstring& valueName, std::wstring& outValue) const;
};

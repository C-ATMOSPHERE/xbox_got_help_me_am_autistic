#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem> 

namespace fs = std::filesystem;


struct BinaryInfo {
    std::wstring name;
    std::wstring exeName;
    bool rename = false;
    std::wstring newExeName;

    bool wasRenamed = false;
};



struct GameExecutableInfo
{
    std::wstring executableName;
    std::wstring fullPath;
    bool wasRenamed = false;
};


struct GameRegistryInfo
{
    std::wstring packageId;       // example: 38985CA0.SomeGame_abc
    std::wstring regKeyPath;      // full registry path to that key
    std::wstring rootPath;        // actual Root value pulled from reg
};


struct GameInstallInfo
{
    std::wstring gameKey;
    std::wstring drive;
    std::wstring directory;
    std::wstring folder;
    std::wstring installPath;
    std::wstring fullPath;

    GameRegistryInfo registryInfo;

    std::vector<GameExecutableInfo> executables;
};



class XboxGameManager 
{
public:
    bool findInstalledGames();
    void printAllGames() const;

    size_t getGamesCount() const { return games.size(); }

    bool createDefaultFile(const std::wstring& path) const;
    bool saveUpdatedFile(const std::wstring& path, const std::vector<BinaryInfo>& binary) const;

    bool findFile(const std::wstring& fileName) const;
    void findExe(const std::wstring& name);

    bool renameExe(const fs::path& sourcePath, const std::wstring& newName);
    void rollbackRenamed(std::vector<BinaryInfo>& binaries);


    void processExes(std::vector<BinaryInfo>& binaries);
    void printLoadedExes(const std::vector<BinaryInfo>& binaries) const;


    std::vector<BinaryInfo> loadFromFile(const std::wstring& path);
    const std::vector<GameInstallInfo>& getGames() const { return games; }
    std::vector<BinaryInfo> loadLocalDatabase() const;


private:
    std::vector<GameInstallInfo> games;
};

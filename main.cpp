#include "xbox/xboxGameManager.h"

int main()
{
    const wchar_t* filename = L"xboxGames.json";

    XboxGameManager manager;

    if (!manager.findInstalledGames())
    {
        std::wcerr << L"Failed to load installed Xbox games." << std::endl;
        return 1;
    }

    std::wcout << L"Loaded " << manager.getGamesCount() << L" games successfully for processing." << std::endl;

    auto exesToFind = manager.loadFromFile(filename);

    if (exesToFind.empty())
    {
        std::wcerr << L"No xboxGames loaded. Attempting to create default xboxGames.json..." << std::endl;

        if (manager.createDefaultFile(filename)) {
            std::wcout << L"Default xboxGames.json created.\nPlease edit it and re-run the program.\nUsing LocalDatabase for now." << std::endl;
            exesToFind = manager.loadLocalDatabase();
        }
        else {
            std::wcerr << L"Failed to create default " << filename << std::endl;
            return 1;
        }
    }


    manager.processExes(exesToFind);
    manager.printLoadedExes(exesToFind);
    manager.printAllGames();
    manager.saveUpdatedFile(filename, exesToFind);

    std::wcout << L"\nDo you want to exit? (y/n): ";
    wchar_t answer;
    std::wcin >> answer;
    if (answer == L'y' || answer == L'Y')
    {
        return 0;
    }

    return 0;
}

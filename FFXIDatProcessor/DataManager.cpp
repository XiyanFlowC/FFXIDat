#include "DataManager.h"

#include <format>
#include <filesystem>

std::string DataManager::gameRootPath = "D:\\Program Files (x86)\\PlayOnline\\SquareEnix\\FINAL FANTASY XI"; // HACK:FIXME: read from regtable?

std::string DataManager::GetPath(int rom, int cat, int no)
{
    if (rom == 1)
        return std::format("{}\\ROM\\{}\\{}.DAT", gameRootPath, cat, no);
    return std::format("{}\\ROM{}\\{}\\{}.DAT", gameRootPath, rom, cat, no);
}

std::string DataManager::GetOutPathConf(int rom, int cat, int no)
{
    auto path = (rom == 1) ? std::format("output\\ROM\\{}", cat) : std::format("output\\ROM{}\\{}", rom, cat);
    if (!std::filesystem::exists(path))
        std::filesystem::create_directories(path);
    return std::format("{}\\{}.DAT", path, no);
}

std::string DataManager::GetOutPath(int rom, int cat, int no)
{
    auto path = (rom == 1) ? std::format("output\\ROM\\{}", cat) : std::format("output\\ROM{}\\{}", rom, cat);
    return std::format("{}\\{}.DAT", path, no);
}

#pragma once

#include <iosfwd>
#include <memory>
#include <string>

class Game;
class Rom;

class GameFactory
{
public:
    GameFactory() = delete;

    static std::shared_ptr<Game> build(const std::shared_ptr<Rom>& file);
    static std::shared_ptr<Game> buildDisassembly(const std::string& iniPath);
    static std::shared_ptr<Game> buildDisassembly(const std::string& rootDir, const std::string& iniPath);
};

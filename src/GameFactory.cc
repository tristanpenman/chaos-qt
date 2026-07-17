#include "GameFactory.h"

#include "games/Sonic2Disassembly.h"
#include "games/Sonic2Rom.h"
#include "games/Sonic3Rom.h"


#include <QDir>
#include <QFileInfo>
#include <QStringList>


namespace {

std::shared_ptr<Game> buildSonic2Disassembly(const QString& iniPath)
{
    std::shared_ptr<Game> game = std::make_shared<Sonic2Disassembly>(iniPath.toStdString());
    if (game->isCompatible()) {
        return game;
    }

    return nullptr;
}

QStringList disassemblyIniCandidates(const QString& path)
{
    const QFileInfo info(path);
    if (!info.isDir()) {
        return {path};
    }

    const QDir dir(path);
    return {
        dir.filePath("SonLVL INI Files/SonLVL.ini"),
        dir.filePath("SonLVL INI Files/SonLVL - S3.ini"),
        dir.filePath("SonLVL INI Files/SonLVL - S&K.ini"),
        dir.filePath("SonLVL.ini"),
    };
}

}  // namespace

std::shared_ptr<Game> GameFactory::build(const std::shared_ptr<Rom>& rom)
{
    // Try Sonic2Rom
    std::shared_ptr<Game> game = std::make_shared<Sonic2Rom>(rom);
    if (game->isCompatible()) {
        return game;
    }

    // Try Sonic3Rom
    game.reset(new Sonic3Rom(rom));
    if (game->isCompatible()) {
        return game;
    }

    return nullptr;
}

std::shared_ptr<Game> GameFactory::buildDisassembly(const std::string& iniPath)
{
    for (const auto& candidate : disassemblyIniCandidates(QString::fromStdString(iniPath))) {
        if (auto game = buildSonic2Disassembly(candidate)) {
            return game;
        }
    }

    return nullptr;
}

std::shared_ptr<Game> GameFactory::buildDisassembly(const std::string& rootDir, const std::string& iniPath)
{
    std::shared_ptr<Game> game = std::make_shared<Sonic2Disassembly>(rootDir, iniPath);
    if (game->isCompatible()) {
        return game;
    }

    return nullptr;
}

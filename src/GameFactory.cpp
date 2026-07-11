#include "games/Sonic2Rom.h"
#include "games/Sonic2Disassembly.h"
#include "games/Sonic3Rom.h"

#include "GameFactory.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

using namespace std;

namespace {

shared_ptr<Game> buildSonic2Disassembly(const QString& iniPath)
{
  shared_ptr<Game> game = make_shared<Sonic2Disassembly>(iniPath.toStdString());
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

shared_ptr<Game> GameFactory::build(const shared_ptr<Rom>& rom)
{
  // try Sonic2Rom
  shared_ptr<Game> game = make_shared<Sonic2Rom>(rom);
  if (game->isCompatible()) {
    return game;
  }

  // try Sonic3Rom
  game.reset(new Sonic3Rom(rom));
  if (game->isCompatible()) {
    return game;
  }

  return nullptr;
}

shared_ptr<Game> GameFactory::buildDisassembly(const string& iniPath)
{
  for (const auto& candidate : disassemblyIniCandidates(QString::fromStdString(iniPath))) {
    if (auto game = buildSonic2Disassembly(candidate)) {
      return game;
    }
  }

  return nullptr;
}

shared_ptr<Game> GameFactory::buildDisassembly(const string& rootDir, const string& iniPath)
{
  shared_ptr<Game> game = make_shared<Sonic2Disassembly>(rootDir, iniPath);
  if (game->isCompatible()) {
    return game;
  }

  return nullptr;
}

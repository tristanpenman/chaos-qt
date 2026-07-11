#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <QDir>
#include <QSettings>

#include "../Game.h"

class Level;

class Sonic2Disassembly : public Game
{
public:
    explicit Sonic2Disassembly(const std::string& iniPath);
    Sonic2Disassembly(const std::string& rootDir, const std::string& iniPath);

    bool isCompatible() override;
    const char* getIdentifier() const override;
    std::vector<std::string> getTitleCards() override;
    std::vector<ProjectResource> getProjectResources() const override;
    std::shared_ptr<Level> loadLevel(unsigned int levelIdx) override;
    bool canRelocateLevels() const override;
    bool canSave() const override;
    bool relocateLevels(bool unsafe) override;
    bool save(unsigned int levelIdx, Level&) override;

private:
    struct LevelEntry
    {
        std::string title;
        QString paletteSpec;
        QString patternsSpec;
        QString blocksSpec;
        QString chunksSpec;
        QString mapSpec;
        QString patternsCompression;
        QString blocksCompression;
        QString chunksCompression;
        QString mapCompression;
    };

    std::vector<LevelEntry> readLevelEntries() const;
    LevelEntry readLevelEntry(const QString& group) const;

    std::vector<char> readPaletteData(const QString& spec) const;
    std::vector<uint8_t> readDataFile(const QString& spec, const QString& compression) const;
    std::vector<uint8_t> decompressKosinski(const QByteArray& data, const QString& description) const;
    QByteArray readFile(const QString& spec) const;
    QString resolvePath(const QString& spec) const;
    QString value(QSettings& settings, const QStringList& keys) const;

    static QString stripSpecArguments(const QString& spec);
    static bool usesKosinskiCompression(const QString& compression);

    QDir m_rootDir;
    QString m_iniPath;
};

#include "Sonic2Disassembly.h"

#include <algorithm>
#include <stdexcept>

#include <QBuffer>
#include <QFile>
#include <QFileInfo>

#include "../KosinskiReader.h"
#include "../Logger.h"
#include "../Palette.h"

#include "Sonic2Level.h"

#undef LOG
#define LOG Logger("Sonic2Disassembly")


namespace {

constexpr size_t kDecompressionBufferSize = 0xFFFF;

QString firstNonEmpty(const QStringList& values)
{
    for (const auto& value : values) {
        if (!value.trimmed().isEmpty()) {
            return value;
        }
    }

    return {};
}

std::vector<uint8_t> toBytes(const QByteArray& data)
{
    return std::vector<uint8_t>(data.begin(), data.end());
}

}  // namespace

Sonic2Disassembly::Sonic2Disassembly(const std::string& iniPath)
    : rootDir_(QFileInfo(QString::fromStdString(iniPath)).absoluteDir())
    , iniPath_(QString::fromStdString(iniPath))
{
}

Sonic2Disassembly::Sonic2Disassembly(const std::string& rootDir, const std::string& iniPath)
    : rootDir_(QString::fromStdString(rootDir))
    , iniPath_(QFileInfo(QString::fromStdString(iniPath)).isAbsolute()
                     ? QString::fromStdString(iniPath)
                     : QDir(QString::fromStdString(rootDir)).filePath(QString::fromStdString(iniPath)))
{
}

bool Sonic2Disassembly::isCompatible()
{
    if (!QFileInfo::exists(iniPath_)) {
        return false;
    }

    return !readLevelEntries().empty();
}

const char* Sonic2Disassembly::getIdentifier() const
{
    return "Sonic2Disassembly";
}

std::vector<std::string> Sonic2Disassembly::getTitleCards()
{
    std::vector<std::string> titleCards;
    for (const auto& entry : readLevelEntries()) {
        titleCards.push_back(entry.title);
    }

    return titleCards;
}

std::vector<ProjectResource> Sonic2Disassembly::getProjectResources() const
{
    std::vector<ProjectResource> resources;
    for (const auto& entry : readLevelEntries()) {
        const auto addResource = [&](const char* type, const QString& spec, const QString& compression) {
            const auto parts = spec.split('|', Qt::SkipEmptyParts);
            for (const auto& part : parts) {
                resources.push_back({entry.title,
                                     type,
                                     resolvePath(part).toStdString(),
                                     compression.toStdString()});
            }
        };

        addResource("Palette", entry.paletteSpec, "none");
        addResource("Patterns", entry.patternsSpec, entry.patternsCompression);
        addResource("Blocks", entry.blocksSpec, entry.blocksCompression);
        addResource("Chunks", entry.chunksSpec, entry.chunksCompression);
        addResource("Map", entry.mapSpec, entry.mapCompression);
    }

    return resources;
}

std::shared_ptr<Level> Sonic2Disassembly::loadLevel(unsigned int levelIdx)
{
    const auto levels = readLevelEntries();
    if (levelIdx >= levels.size()) {
        throw std::runtime_error("Invalid level index");
    }

    const auto& entry = levels[levelIdx];

    LOG() << "Loading " << entry.title;
    LOG() << "Palette spec: " << entry.paletteSpec.toStdString();
    LOG() << "Patterns spec: " << entry.patternsSpec.toStdString();
    LOG() << "Blocks spec: " << entry.blocksSpec.toStdString();
    LOG() << "Chunks spec: " << entry.chunksSpec.toStdString();
    LOG() << "Map spec: " << entry.mapSpec.toStdString();

    return std::make_shared<Sonic2Level>(readPaletteData(entry.paletteSpec),
                                    readDataFile(entry.patternsSpec, entry.patternsCompression),
                                    readDataFile(entry.blocksSpec, entry.blocksCompression),
                                    readDataFile(entry.chunksSpec, entry.chunksCompression),
                                    readDataFile(entry.mapSpec, entry.mapCompression));
}

bool Sonic2Disassembly::canRelocateLevels() const
{
    return false;
}

bool Sonic2Disassembly::canSave() const
{
    return false;
}

bool Sonic2Disassembly::relocateLevels(bool)
{
    return false;
}

bool Sonic2Disassembly::save(unsigned int, Level&)
{
    return false;
}

std::vector<Sonic2Disassembly::LevelEntry> Sonic2Disassembly::readLevelEntries() const
{
    QSettings settings(iniPath_, QSettings::IniFormat);
    std::vector<LevelEntry> entries;

    const auto groups = settings.childGroups();
    for (const auto& group : groups) {
        const LevelEntry entry = readLevelEntry(group);
        if (!entry.patternsSpec.isEmpty() && !entry.blocksSpec.isEmpty() &&
            !entry.chunksSpec.isEmpty() && !entry.mapSpec.isEmpty() &&
            !entry.paletteSpec.isEmpty()) {
            entries.push_back(entry);
        }
    }

    return entries;
}

Sonic2Disassembly::LevelEntry Sonic2Disassembly::readLevelEntry(const QString& group) const
{
    QSettings settings(iniPath_, QSettings::IniFormat);
    settings.beginGroup(group);

    LevelEntry entry;
    entry.title = settings.value("title", group).toString().toStdString();
    entry.paletteSpec = value(settings, {"palette", "palettes"});
    entry.patternsSpec = value(settings, {"patterns", "tiles", "art", "arttiles"});
    entry.blocksSpec = value(settings, {"blocks", "block", "map16"});
    entry.chunksSpec = value(settings, {"chunks", "chunk", "map128"});
    entry.mapSpec = value(settings, {"map", "layout", "level"});
    entry.patternsCompression = value(settings, {"patternscmp", "tilescmp", "artcmp", "compression"});
    entry.blocksCompression = value(settings, {"blockscmp", "map16cmp", "compression"});
    entry.chunksCompression = value(settings, {"chunkscmp", "map128cmp", "compression"});
    entry.mapCompression = value(settings, {"mapcmp", "layoutcmp", "levelcmp", "compression"});

    return entry;
}

std::vector<char> Sonic2Disassembly::readPaletteData(const QString& spec) const
{
    std::vector<char> paletteData(Palette::kPaletteSizeInRom * 4, 0);
    const auto parts = spec.split('|', Qt::SkipEmptyParts);

    if (parts.isEmpty()) {
        throw std::runtime_error("Missing palette data");
    }

    size_t nextPaletteByte = 0;
    for (const auto& part : parts) {
        const auto args = part.split(':');
        const QByteArray data = readFile(args.value(0));

        size_t srcOffset = 0;
        size_t destOffset = nextPaletteByte;
        size_t byteCount = std::min(static_cast<size_t>(data.size()), paletteData.size() - destOffset);

        if (args.size() >= 4) {
            srcOffset = args.value(1).toUInt() * Palette::kBytesPerColor;
            destOffset = args.value(2).toUInt() * Palette::kBytesPerColor;
            byteCount = args.value(3).toUInt() * Palette::kBytesPerColor;
        }

        if (srcOffset + byteCount > static_cast<size_t>(data.size()) ||
            destOffset + byteCount > paletteData.size()) {
            throw std::runtime_error("Palette spec is out of range");
        }

        copy(data.begin() + static_cast<qsizetype>(srcOffset),
             data.begin() + static_cast<qsizetype>(srcOffset + byteCount),
             paletteData.begin() + static_cast<ptrdiff_t>(destOffset));
        nextPaletteByte = destOffset + byteCount;
    }

    return paletteData;
}

std::vector<uint8_t> Sonic2Disassembly::readDataFile(const QString& spec, const QString& compression) const
{
    const QByteArray data = readFile(spec);
    if (usesKosinskiCompression(compression)) {
        return decompressKosinski(data, stripSpecArguments(spec));
    }

    return toBytes(data);
}

std::vector<uint8_t> Sonic2Disassembly::decompressKosinski(const QByteArray& data, const QString& description) const
{
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);

    std::vector<uint8_t> output(kDecompressionBufferSize);
    KosinskiReader reader;
    const auto result = reader.decompress(buffer, output.data(), output.size());
    if (!result.first) {
        throw std::runtime_error("Kosinski decompression buffer too small for " + description.toStdString());
    }

    output.resize(result.second);
    return output;
}

QByteArray Sonic2Disassembly::readFile(const QString& spec) const
{
    QFile file(resolvePath(spec));
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Failed to open disassembly file: " + file.fileName().toStdString());
    }

    return file.readAll();
}

QString Sonic2Disassembly::resolvePath(const QString& spec) const
{
    const QString path = stripSpecArguments(spec);
    if (QFileInfo(path).isAbsolute()) {
        return path;
    }

    return rootDir_.filePath(path);
}

QString Sonic2Disassembly::value(QSettings& settings, const QStringList& keys) const
{
    QStringList values;
    for (const auto& key : keys) {
        values.push_back(settings.value(key).toString());
    }

    return firstNonEmpty(values);
}

QString Sonic2Disassembly::stripSpecArguments(const QString& spec)
{
    return spec.split(':').value(0).trimmed();
}

bool Sonic2Disassembly::usesKosinskiCompression(const QString& compression)
{
    const QString value = compression.trimmed().toLower();
    if (value.isEmpty()) {
        return true;
    }

    return value != "none" && value != "raw" && value != "uncompressed";
}

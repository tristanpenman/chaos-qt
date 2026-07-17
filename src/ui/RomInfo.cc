#include "RomInfo.h"

#include <QFormLayout>
#include <QLabel>

#include "../Game.h"
#include "../Rom.h"


RomInfo::RomInfo(QWidget* parent, Rom& rom, Game& game)
    : QDialog(parent)
{
    auto layout = new QFormLayout(this);
    setLayout(layout);

    // Identified as
    auto identifiedAsLabel = new QLabel(this);
    identifiedAsLabel->setText(tr("Identified as:"));
    auto identifiedAsField = new QLabel(this);
    identifiedAsField->setText(game.getIdentifier());
    layout->addRow(identifiedAsLabel, identifiedAsField);

    // File size
    auto fileSizeLabel = new QLabel(this);
    fileSizeLabel->setText(tr("File size:"));
    auto fileSizeField = new QLabel(this);
    auto fileSize = rom.getSize();
    fileSizeField->setText(QString::number(fileSize) + tr(" bytes"));
    layout->addRow(fileSizeLabel, fileSizeField);

    // Address range
    auto addrRangeLabel = new QLabel(this);
    addrRangeLabel->setText(tr("Address range:"));
    auto addrRangeField = new QLabel(this);
    auto addrRange = rom.readAddrRange();
    addrRangeField->setText(QStringLiteral("0x%1 (%2)").arg(
                            QStringLiteral("%1").arg(addrRange, 1, 16).toUpper(),
                            QString::number(addrRange) + " bytes"));
    layout->addRow(addrRangeLabel, addrRangeField);

    // Checksum in rom
    auto romChecksumLabel = new QLabel(this);
    romChecksumLabel->setText(tr("Rom checksum:"));
    auto romChecksumField = new QLabel(this);
    auto romChecksum = rom.readChecksum();
    romChecksumField->setText(QString::number(romChecksum));
    layout->addRow(romChecksumLabel, romChecksumField);

    // Calculated checksum
    auto calculatedChecksumLabel = new QLabel(this);
    calculatedChecksumLabel->setText(tr("Calculated checksum:"));
    auto calculatedChecksumField = new QLabel(this);
    auto calculatedChecksum = rom.calculateChecksum();
    calculatedChecksumField->setText(QString::number(calculatedChecksum));
    layout->addRow(calculatedChecksumLabel, calculatedChecksumField);
}

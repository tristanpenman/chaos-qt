#include "PaletteInspector.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "../Level.h"
#include "../Palette.h"


PaletteInspector::PaletteInspector(QWidget* parent, const std::shared_ptr<Level>& level)
    : QDialog(parent)
    , level_(level)
{
    setWindowFlag(Qt::WindowStaysOnTopHint);

    QVBoxLayout* vbox = new QVBoxLayout();
    vbox->setContentsMargins(8, 8, 8, 8);
    vbox->setSpacing(0);

    for (size_t r = 0; r < level_->getPaletteCount(); r++) {
        const Palette& palette = level_->getPalette(r);

        QHBoxLayout* hbox = new QHBoxLayout();
        hbox->setSpacing(0);

        for (size_t c = 0; c < palette.getColorCount(); c++) {
            const Palette::Color& color = palette.getColor(c);
            const QString stylesheet = QStringLiteral("background: rgb(%1,%2,%3)")
          .arg(color.r)
          .arg(color.g)
          .arg(color.b);

            QWidget* cell = new QWidget();
            cell->setMinimumSize(20, 20);
            cell->setStyleSheet(stylesheet);

            hbox->addWidget(cell);
        }

        vbox->addLayout(hbox);
    }

    setLayout(vbox);
}

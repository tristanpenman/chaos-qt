#include "LevelSelect.h"

#include <iostream>

#include <QEvent>
#include <QHBoxLayout>
#include <QListView>
#include <QMouseEvent>
#include <QPushButton>
#include <QStringListModel>
#include <QVBoxLayout>

#include "../Game.h"

LevelSelect::LevelSelect(QWidget* parent, const std::shared_ptr<Game>& game)
    : QDialog(parent)
    , m_game(game)
{
    setModal(true);
    setWindowTitle(tr("Level Select"));

    QStringList stringList;
    auto titleCards = game->getTitleCards();
    for (const auto& titleCard : titleCards) {
        stringList << QString::fromStdString(titleCard);
    }

    // create model
    auto model = new QStringListModel(this);
    model->setStringList(stringList);

    // create list view
    m_listView = new QListView();
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setModel(model);

    // enable OK button when selection is valid
    connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &LevelSelect::selectionChanged);

    m_listView->viewport()->installEventFilter(this);

    m_okButton = new QPushButton(tr("OK"));
    m_okButton->setDisabled(true);
    connect(m_okButton, &QPushButton::clicked, this, &LevelSelect::ok);

    auto cancelButton = new QPushButton(tr("Cancel"));
    connect(cancelButton, &QPushButton::clicked, this, &LevelSelect::cancel);

    auto hbox = new QHBoxLayout();
    hbox->addWidget(m_okButton);
    hbox->addWidget(cancelButton);

    auto vbox = new QVBoxLayout();
    vbox->addWidget(m_listView);
    vbox->addLayout(hbox);
    vbox->setContentsMargins(20, 20, 20, 15);
    vbox->setSizeConstraint(QLayout::SetFixedSize);

    setLayout(vbox);
}

void LevelSelect::ok(bool)
{
    auto currentIndex = m_listView->currentIndex();
    if (currentIndex.isValid()) {
        emit levelSelected(currentIndex.row());
        accept();
    }
}

void LevelSelect::cancel(bool)
{
    reject();
}

void LevelSelect::selectionChanged(const QItemSelection& selection)
{
    m_okButton->setDisabled(selection.length() == 0);
}

bool LevelSelect::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_listView->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        ok(true);
        return true;
    }

    return QDialog::eventFilter(watched, event);
}

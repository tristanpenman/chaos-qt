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
    , game_(game)
{
    setModal(true);
    setWindowTitle(tr("Level Select"));

    QStringList stringList;
    auto titleCards = game->getTitleCards();
    for (const auto& titleCard : titleCards) {
        stringList << QString::fromStdString(titleCard);
    }

    auto model = new QStringListModel(this);
    model->setStringList(stringList);

    listView_ = new QListView();
    listView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listView_->setModel(model);

    // Enable OK button when selection is valid
    connect(listView_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &LevelSelect::selectionChanged);

    listView_->viewport()->installEventFilter(this);

    okButton_ = new QPushButton(tr("OK"));
    okButton_->setDisabled(true);
    connect(okButton_, &QPushButton::clicked, this, &LevelSelect::ok);

    auto cancelButton = new QPushButton(tr("Cancel"));
    connect(cancelButton, &QPushButton::clicked, this, &LevelSelect::cancel);

    auto hbox = new QHBoxLayout();
    hbox->addWidget(okButton_);
    hbox->addWidget(cancelButton);

    auto vbox = new QVBoxLayout();
    vbox->addWidget(listView_);
    vbox->addLayout(hbox);
    vbox->setContentsMargins(20, 20, 20, 15);
    vbox->setSizeConstraint(QLayout::SetFixedSize);

    setLayout(vbox);
}

void LevelSelect::ok(bool)
{
    auto currentIndex = listView_->currentIndex();
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
    okButton_->setDisabled(selection.length() == 0);
}

bool LevelSelect::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == listView_->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        ok(true);
        return true;
    }

    return QDialog::eventFilter(watched, event);
}

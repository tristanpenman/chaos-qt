#pragma once

#include <memory>

#include <QDialog>
#include <QItemSelection>

class QListView;
class QPushButton;

class Game;

class LevelSelect : public QDialog
{
    Q_OBJECT

public:
    LevelSelect(QWidget* parent, const std::shared_ptr<Game>& game);

signals:
    void levelSelected(int levelIdx);

private:
    std::shared_ptr<Game> game_;
    QListView* listView_;
    QPushButton* okButton_;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void ok(bool);
    void cancel(bool);
    void selectionChanged(const QItemSelection& selection);
};

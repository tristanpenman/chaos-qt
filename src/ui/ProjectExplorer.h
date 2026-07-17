#pragma once

#include <vector>

#include <QDialog>

#include "../Game.h"

class QTableWidget;

class ProjectExplorer : public QDialog
{
public:
    explicit ProjectExplorer(QWidget* parent, const std::vector<ProjectResource>& resources);

private:
    QTableWidget* table_;
};

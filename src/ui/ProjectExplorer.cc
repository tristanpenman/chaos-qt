#include "ProjectExplorer.h"

#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>


namespace {

constexpr int kColumnCount = 4;

QString text(const std::string& value)
{
    return QString::fromStdString(value);
}

}  // namespace

ProjectExplorer::ProjectExplorer(QWidget* parent, const std::vector<ProjectResource>& resources)
    : QDialog(parent)
    , table_(new QTableWidget(this))
{
    setWindowTitle(tr("Project Explorer"));
    resize(900, 500);

    table_->setColumnCount(kColumnCount);
    table_->setHorizontalHeaderLabels({tr("Level"), tr("Type"), tr("Path"), tr("Compression")});
    table_->setRowCount(static_cast<int>(resources.size()));
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSortingEnabled(false);

    for (int row = 0; row < static_cast<int>(resources.size()); row++) {
        const auto& resource = resources.at(static_cast<size_t>(row));
        table_->setItem(row, 0, new QTableWidgetItem(text(resource.level)));
        table_->setItem(row, 1, new QTableWidgetItem(text(resource.type)));
        table_->setItem(row, 2, new QTableWidgetItem(text(resource.path)));
        table_->setItem(row, 3, new QTableWidgetItem(text(resource.compression)));
    }

    table_->setSortingEnabled(true);
    table_->sortByColumn(0, Qt::AscendingOrder);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table_->verticalHeader()->setVisible(false);

    auto* layout = new QVBoxLayout();
    layout->addWidget(table_);
    setLayout(layout);
}

#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

#include "ProjectExplorer.h"

namespace {

constexpr int ColumnCount = 4;

QString text(const std::string& value)
{
    return QString::fromStdString(value);
}

}  // namespace

ProjectExplorer::ProjectExplorer(QWidget* parent, const std::vector<ProjectResource>& resources)
    : QDialog(parent)
    , m_table(new QTableWidget(this))
{
    setWindowTitle(tr("Project Explorer"));
    resize(900, 500);

    m_table->setColumnCount(ColumnCount);
    m_table->setHorizontalHeaderLabels({tr("Level"), tr("Type"), tr("Path"), tr("Compression")});
    m_table->setRowCount(static_cast<int>(resources.size()));
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(false);

    for (int row = 0; row < static_cast<int>(resources.size()); row++) {
        const auto& resource = resources.at(static_cast<size_t>(row));
        m_table->setItem(row, 0, new QTableWidgetItem(text(resource.level)));
        m_table->setItem(row, 1, new QTableWidgetItem(text(resource.type)));
        m_table->setItem(row, 2, new QTableWidgetItem(text(resource.path)));
        m_table->setItem(row, 3, new QTableWidgetItem(text(resource.compression)));
    }

    m_table->setSortingEnabled(true);
    m_table->sortByColumn(0, Qt::AscendingOrder);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);

    auto* layout = new QVBoxLayout();
    layout->addWidget(m_table);
    setLayout(layout);
}

#include "sidebar_widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void SidebarWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Search box
    searchBox_ = new QLineEdit(this);
    searchBox_->setPlaceholderText("🔍  搜索工具...");
    searchBox_->setClearButtonEnabled(true);
    connect(searchBox_, &QLineEdit::textChanged,
            this, &SidebarWidget::onSearchTextChanged);
    layout->addWidget(searchBox_);

    // Tool tree
    treeWidget_ = new QTreeWidget(this);
    treeWidget_->setHeaderHidden(true);
    treeWidget_->setRootIsDecorated(true);
    treeWidget_->setIndentation(16);
    treeWidget_->setAnimated(true);
    treeWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
    treeWidget_->setFocusPolicy(Qt::NoFocus);
    connect(treeWidget_, &QTreeWidget::itemClicked,
            this, &SidebarWidget::onItemClicked);
    connect(treeWidget_, &QTreeWidget::itemDoubleClicked,
            this, &SidebarWidget::onItemDoubleClicked);
    layout->addWidget(treeWidget_);

    // Action buttons
    auto *btnLayout = new QHBoxLayout();
    runButton_ = new QPushButton("▶  运行", this);
    runButton_->setEnabled(false);
    connect(runButton_, &QPushButton::clicked,
            this, &SidebarWidget::onRunClicked);
    btnLayout->addWidget(runButton_);

    stopButton_ = new QPushButton("■  停止", this);
    stopButton_->setEnabled(false);
    connect(stopButton_, &QPushButton::clicked,
            this, [this]() {
        if (!currentToolName_.isEmpty()) {
            emit stopRequested(currentToolName_);
        }
    });
    btnLayout->addWidget(stopButton_);

    layout->addLayout(btnLayout);
}

void SidebarWidget::addTool(const ToolManifest &manifest)
{
    QTreeWidgetItem *categoryItem = findOrCreateCategory(manifest.category);

    auto *toolItem = new QTreeWidgetItem(categoryItem);
    toolItem->setText(0, manifest.displayName);
    toolItem->setToolTip(0, manifest.description);
    toolItem->setData(0, Qt::UserRole, manifest.name);

    // Try to set an icon
    QIcon icon = QIcon::fromTheme(manifest.icon);
    if (!icon.isNull()) {
        toolItem->setIcon(0, icon);
    }

    categoryItem->addChild(toolItem);
    categoryItem->setExpanded(true);
}

void SidebarWidget::clear()
{
    treeWidget_->clear();
    categoryItems_.clear();
    currentToolName_.clear();
    runButton_->setEnabled(false);
    stopButton_->setEnabled(false);
}

void SidebarWidget::setFilter(const QString &text)
{
    searchBox_->setText(text);
}

QTreeWidgetItem *SidebarWidget::findOrCreateCategory(const QString &category)
{
    auto it = categoryItems_.find(category);
    if (it != categoryItems_.end()) {
        return *it;
    }

    auto *item = new QTreeWidgetItem(treeWidget_);
    item->setText(0, category);
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);

    // Format category names nicely
    QString displayCategory = category;
    displayCategory[0] = displayCategory[0].toUpper();
    item->setText(0, displayCategory);

    QFont font = item->font(0);
    font.setBold(true);
    item->setFont(0, font);

    categoryItems_[category] = item;
    return item;
}

void SidebarWidget::onItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item || item->parent() == nullptr) {
        return; // Category header — not selectable
    }

    currentToolName_ = item->data(0, Qt::UserRole).toString();
    runButton_->setEnabled(true);
    stopButton_->setEnabled(true);
    emit toolSelected(currentToolName_);
}

void SidebarWidget::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item || item->parent() == nullptr) {
        return; // Category header
    }

    currentToolName_ = item->data(0, Qt::UserRole).toString();
    emit runRequested(currentToolName_);
}

void SidebarWidget::onSearchTextChanged(const QString &text)
{
    for (int i = 0; i < treeWidget_->topLevelItemCount(); ++i) {
        QTreeWidgetItem *category = treeWidget_->topLevelItem(i);
        bool categoryVisible = false;

        for (int j = 0; j < category->childCount(); ++j) {
            QTreeWidgetItem *tool = category->child(j);
            bool matches = text.isEmpty()
                || tool->text(0).contains(text, Qt::CaseInsensitive)
                || tool->toolTip(0).contains(text, Qt::CaseInsensitive);
            tool->setHidden(!matches);
            if (matches) categoryVisible = true;
        }

        category->setHidden(!categoryVisible);
    }
}

void SidebarWidget::onRunClicked()
{
    if (!currentToolName_.isEmpty()) {
        emit runRequested(currentToolName_);
    }
}

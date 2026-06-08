#include "dashboard_widget.h"
#include "tool_card.h"
#include "core/tool_manager.h"
#include "core/tool_instance.h"

#include <QVBoxLayout>
#include <QScrollArea>

DashboardWidget::DashboardWidget(ToolManager *manager, QWidget *parent)
    : QWidget(parent)
    , manager_(manager)
{
    setupUi();
}

void DashboardWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Header
    auto *header = new QLabel("<h2>📊 仪表盘</h2>", this);
    mainLayout->addWidget(header);

    // Empty state
    emptyLabel_ = new QLabel(this);
    emptyLabel_->setText(
        "<div style='text-align: center; margin-top: 60px; color: #888;'>"
        "<p style='font-size: 48px;'>🛠️</p>"
        "<p>暂无运行中的工具。<br/>从侧边栏选择一个工具，点击 <b>运行</b> 即可启动。</p>"
        "</div>"
    );
    emptyLabel_->setAlignment(Qt::AlignCenter);

    // Scrollable card area
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameStyle(QFrame::NoFrame);

    cardContainer_ = new QWidget(scrollArea);
    cardLayout_ = new QGridLayout(cardContainer_);
    cardLayout_->setContentsMargins(12, 8, 12, 8);
    cardLayout_->setSpacing(12);

    auto *cardOuterLayout = new QVBoxLayout(cardContainer_);
    cardOuterLayout->addLayout(cardLayout_);
    cardOuterLayout->addStretch();

    scrollArea->setWidget(cardContainer_);

    auto *stackLayout = new QVBoxLayout();
    stackLayout->addWidget(emptyLabel_);
    stackLayout->addWidget(scrollArea);
    mainLayout->addLayout(stackLayout);
}

void DashboardWidget::addToolCard(const QString &name, ToolInstance *instance)
{
    // Remove existing card if any
    removeToolCard(name);

    auto *card = new ToolCard(name, instance, cardContainer_);
    connect(card, &ToolCard::runClicked, this, &DashboardWidget::runRequested);
    connect(card, &ToolCard::stopClicked, this, &DashboardWidget::stopRequested);

    cards_[name] = card;
    emptyLabel_->setVisible(false);
    relayoutCards();
}

void DashboardWidget::updateToolState(const QString &name, ToolState state)
{
    Q_UNUSED(state);
    auto it = cards_.find(name);
    if (it != cards_.end()) {
        it.value()->refresh();
    }
}

void DashboardWidget::removeToolCard(const QString &name)
{
    auto it = cards_.find(name);
    if (it != cards_.end()) {
        cardLayout_->removeWidget(it.value());
        it.value()->deleteLater();
        cards_.erase(it);
        relayoutCards();
    }

    if (cards_.isEmpty()) {
        emptyLabel_->setVisible(true);
    }
}

void DashboardWidget::relayoutCards()
{
    // Re-layout cards into a grid that fills width
    int cardWidth = 220;
    int availableWidth = cardContainer_->width() - 24;
    int cols = qMax(1, availableWidth / cardWidth);

    int row = 0, col = 0;
    for (auto *card : cards_) {
        cardLayout_->addWidget(card, row, col);
        col++;
        if (col >= cols) {
            col = 0;
            row++;
        }
    }
}

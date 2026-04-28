#include "file_explorer.h"
#include "../theme/theme.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>

FileExplorer::FileExplorer(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header
    auto* header = new QLabel("  EXPLORER");
    header->setFixedHeight(36);
    header->setFont(Theme::statusFont());
    header->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  background: %2;"
        "  padding-left: 12px;"
        "  letter-spacing: 1.5px;"
        "  border-bottom: 1px solid %3;"
        "}"
    ).arg(Theme::TextMuted.name(QColor::HexArgb),
          Theme::SidebarBg.name(),
          Theme::Border.name(QColor::HexArgb)));
    layout->addWidget(header);

    // Tree
    model_ = new QFileSystemModel(this);
    model_->setReadOnly(true);

    tree_ = new QTreeView(this);
    tree_->setModel(model_);
    tree_->setHeaderHidden(true);
    // Only show name column
    tree_->hideColumn(1);
    tree_->hideColumn(2);
    tree_->hideColumn(3);
    tree_->setAnimated(true);
    tree_->setIndentation(16);
    tree_->setFont(Theme::sidebarFont());

    layout->addWidget(tree_);

    connect(tree_, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
        QString path = model_->filePath(index);
        if (!model_->isDir(index)) {
            emit fileDoubleClicked(path);
        }
    });

    applyStyle();
}

void FileExplorer::applyStyle() {
    tree_->setStyleSheet(QString(
        "QTreeView {"
        "  background: %1;"
        "  color: %2;"
        "  border: none;"
        "  outline: none;"
        "  font-family: 'JetBrains Mono', 'Consolas', monospace;"
        "  font-size: 12px;"
        "}"
        "QTreeView::item {"
        "  height: 28px;"
        "  padding-left: 4px;"
        "  border: none;"
        "  border-left: 2px solid transparent;"
        "}"
        "QTreeView::item:selected {"
        "  background: rgba(0, 255, 156, 0.08);"
        "  color: %3;"
        "  border-left: 2px solid %4;"
        "}"
        "QTreeView::item:hover {"
        "  background: rgba(255, 255, 255, 0.03);"
        "}"
        "QTreeView::branch {"
        "  background: %1;"
        "}"
        "QTreeView::branch:has-children:!has-siblings:closed,"
        "QTreeView::branch:closed:has-children:has-siblings {"
        "  image: none;"
        "}"
        "QTreeView::branch:open:has-children:!has-siblings,"
        "QTreeView::branch:open:has-children:has-siblings {"
        "  image: none;"
        "}"
        // Scrollbar
        "QScrollBar:vertical {"
        "  width: 4px;"
        "  background: transparent;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: rgba(255,255,255,0.1);"
        "  border-radius: 2px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: rgba(255,255,255,0.2);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
    ).arg(Theme::SidebarBg.name(),
          Theme::TextSecondary.name(QColor::HexArgb),
          Theme::TextPrimary.name(),
          Theme::Accent.name()));

    setStyleSheet(QString("background: %1;").arg(Theme::SidebarBg.name()));
}

void FileExplorer::setRootPath(const QString& path) {
    model_->setRootPath(path);
    tree_->setRootIndex(model_->index(path));
}

QString FileExplorer::rootPath() const {
    return model_->rootPath();
}

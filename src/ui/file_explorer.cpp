#include "file_explorer.h"
#include "../theme/theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

FileExplorer::FileExplorer(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Toolbar (header + action buttons)
    layout->addWidget(createToolbar());

    // Tree view
    model_ = new QFileSystemModel(this);
    model_->setReadOnly(false);
    model_->setNameFilterDisables(false);

    tree_ = new QTreeView(this);
    tree_->setModel(model_);
    tree_->setHeaderHidden(true);
    tree_->hideColumn(1);
    tree_->hideColumn(2);
    tree_->hideColumn(3);
    tree_->setAnimated(true);
    tree_->setIndentation(16);
    tree_->setFont(Theme::sidebarFont());
    tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_->setDragEnabled(false);
    tree_->setFocusPolicy(Qt::NoFocus);

    layout->addWidget(tree_);

    connect(tree_, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
        QString path = model_->filePath(index);
        if (!model_->isDir(index)) {
            emit fileDoubleClicked(path);
        }
    });

    applyStyle();
}

QWidget* FileExplorer::createToolbar() {
    auto* toolbar = new QWidget(this);
    auto* toolLayout = new QHBoxLayout(toolbar);
    toolLayout->setContentsMargins(14, 8, 8, 8);
    toolLayout->setSpacing(2);

    projectLabel_ = new QLabel("EXPLORER");
    projectLabel_->setFont(Theme::statusFont());
    projectLabel_->setStyleSheet(QString(
        "color: %1; letter-spacing: 1.2px; font-weight: 500;"
    ).arg(Theme::TextMuted.name(QColor::HexArgb)));

    toolLayout->addWidget(projectLabel_);
    toolLayout->addStretch();

    // Action buttons
    auto* newFileBtn = makeToolButton(QString::fromUtf8("\xF0\x9F\x93\x84"), "New File (Ctrl+N)");
    auto* newFolderBtn = makeToolButton(QString::fromUtf8("\xF0\x9F\x93\x81"), "New Folder");
    auto* refreshBtn = makeToolButton(QString::fromUtf8("\xE2\x86\xBB"), "Refresh");
    auto* collapseBtn = makeToolButton(QString::fromUtf8("\xE2\x8C\x83"), "Collapse All");

    // Use simpler text that renders reliably on Windows
    newFileBtn->setText("+");
    newFileBtn->setToolTip("New File");
    newFolderBtn->setText("▢");
    newFolderBtn->setToolTip("New Folder");
    refreshBtn->setText("↻");
    refreshBtn->setToolTip("Refresh");
    collapseBtn->setText("⌃");
    collapseBtn->setToolTip("Collapse All");

    toolLayout->addWidget(newFileBtn);
    toolLayout->addWidget(newFolderBtn);
    toolLayout->addWidget(refreshBtn);
    toolLayout->addWidget(collapseBtn);

    connect(newFileBtn, &QPushButton::clicked, this, &FileExplorer::onNewFile);
    connect(newFolderBtn, &QPushButton::clicked, this, &FileExplorer::onNewFolder);
    connect(refreshBtn, &QPushButton::clicked, this, &FileExplorer::onRefresh);
    connect(collapseBtn, &QPushButton::clicked, this, &FileExplorer::onCollapseAll);

    toolbar->setFixedHeight(36);
    toolbar->setStyleSheet(QString(
        "background: %1; border-bottom: 1px solid %2;"
    ).arg(Theme::SidebarBg.name(), Theme::Border.name(QColor::HexArgb)));

    return toolbar;
}

QPushButton* FileExplorer::makeToolButton(const QString& text, const QString& tooltip) {
    auto* btn = new QPushButton(text, this);
    btn->setToolTip(tooltip);
    btn->setFixedSize(24, 24);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  background: transparent;"
        "  color: %1;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 14px;"
        "  padding: 0;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(255, 255, 255, 0.08);"
        "  color: %2;"
        "}"
    ).arg(Theme::TextMuted.name(QColor::HexArgb),
          Theme::TextPrimary.name()));
    return btn;
}

void FileExplorer::applyStyle() {
    tree_->setStyleSheet(QString(
        "QTreeView {"
        "  background: %1;"
        "  color: %2;"
        "  border: none;"
        "  outline: none;"
        "  font-size: 12px;"
        "}"
        "QTreeView::item {"
        "  height: 26px;"
        "  padding-left: 4px;"
        "  border: none;"
        "  border-left: 2px solid transparent;"
        "  border-radius: 0px;"
        "}"
        "QTreeView::item:selected {"
        "  background: rgba(255, 255, 255, 0.06);"
        "  color: %3;"
        "  border-left: 2px solid %4;"
        "}"
        "QTreeView::item:hover:!selected {"
        "  background: rgba(255, 255, 255, 0.03);"
        "}"
        "QTreeView::branch {"
        "  background: %1;"
        "}"
        "QTreeView::branch:has-children:closed {"
        "  image: none;"
        "}"
        "QTreeView::branch:has-children:open {"
        "  image: none;"
        "}"
        // Ultra-thin scrollbar
        "QScrollBar:vertical {"
        "  width: 4px;"
        "  background: transparent;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: rgba(255,255,255,0.08);"
        "  border-radius: 2px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: rgba(255,255,255,0.15);"
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
    rootPath_ = path;
    model_->setRootPath(path);
    tree_->setRootIndex(model_->index(path));

    // Update project label to show folder name
    QDir dir(path);
    projectLabel_->setText(dir.dirName().toUpper());
}

QString FileExplorer::rootPath() const {
    return rootPath_;
}

QString FileExplorer::currentDirectory() const {
    QModelIndex idx = tree_->currentIndex();
    if (idx.isValid()) {
        QString path = model_->filePath(idx);
        if (model_->isDir(idx)) return path;
        return QFileInfo(path).absolutePath();
    }
    return rootPath_;
}

void FileExplorer::onNewFile() {
    QString dir = currentDirectory();
    if (dir.isEmpty()) return;

    bool ok;
    QString name = QInputDialog::getText(this, "New File", "File name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    QString fullPath = dir + "/" + name;
    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.close();
        // Select the new file
        QModelIndex idx = model_->index(fullPath);
        tree_->setCurrentIndex(idx);
        emit fileDoubleClicked(fullPath);
    } else {
        QMessageBox::warning(this, "Error", "Could not create file: " + name);
    }
}

void FileExplorer::onNewFolder() {
    QString dir = currentDirectory();
    if (dir.isEmpty()) return;

    bool ok;
    QString name = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    QDir d(dir);
    if (!d.mkdir(name)) {
        QMessageBox::warning(this, "Error", "Could not create folder: " + name);
    }
}

void FileExplorer::onRefresh() {
    if (!rootPath_.isEmpty()) {
        model_->setRootPath("");
        model_->setRootPath(rootPath_);
        tree_->setRootIndex(model_->index(rootPath_));
    }
}

void FileExplorer::onCollapseAll() {
    tree_->collapseAll();
}

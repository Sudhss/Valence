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
#include <QTimer>
#include <QMenu>
#include <QAction>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QClipboard>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QKeyEvent>

namespace {

// Names Windows will not accept as a file name. Checked as the user types so
// the rename cannot silently fail with no explanation.
bool isReservedName(const QString& name) {
    static const QStringList reserved = {
        "CON", "PRN", "AUX", "NUL",
        "COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9",
        "LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"};
    const QString stem = name.section(QLatin1Char('.'), 0, 0).toUpper();
    return reserved.contains(stem);
}

// The inline rename editor. QFileSystemModel supplies a bare QLineEdit that
// inherits nothing from the theme and accepts characters the filesystem will
// reject, so both are fixed here.
class NameEditDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override {
        QWidget* w = QStyledItemDelegate::createEditor(parent, option, index);
        if (auto* edit = qobject_cast<QLineEdit*>(w)) {
            // Allow the empty string through as intermediate, so the user can
            // clear the field and retype.
            static const QRegularExpression allowed(
                QStringLiteral(R"(^[^<>:"/\\|?*\x{00}-\x{1F}]{0,255}$)"));
            edit->setValidator(new QRegularExpressionValidator(allowed, edit));
            edit->setFrame(false);
            edit->setStyleSheet(QString(
                "QLineEdit { background: %1; color: %2; border: 1px solid %3;"
                "  border-radius: 3px; padding: 1px 3px; selection-background-color: %4; }"
            ).arg(Theme::EditorBg.name(),
                  Theme::TextPrimary.name(),
                  Theme::Accent.name(),
                  Theme::SelectionBg.name(QColor::HexArgb)));
        }
        return w;
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override {
        auto* edit = qobject_cast<QLineEdit*>(editor);
        if (edit) {
            const QString name = edit->text().trimmed();
            // Committing one of these produces an opaque filesystem error, so
            // discard the edit and leave the original name in place instead.
            if (name.isEmpty() || isReservedName(name) || name.endsWith(QLatin1Char('.'))) {
                return;
            }
        }
        QStyledItemDelegate::setModelData(editor, model, index);
    }
};

} // namespace

FileExplorer::FileExplorer(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Toolbar (header + action buttons)
    layout->addWidget(createToolbar());

    // Stack for switching between No Folder and Tree View
    stack_ = new QStackedWidget(this);

    // No Folder view
    auto* noFolderWidget = new QWidget(this);
    auto* noFolderLayout = new QVBoxLayout(noFolderWidget);
    noFolderLayout->setAlignment(Qt::AlignTop);
    noFolderLayout->setContentsMargins(16, 16, 16, 16);
    
    auto* openFolderBtn = new QPushButton("Open Folder", this);
    openFolderBtn->setFont(Theme::statusFont());
    openFolderBtn->setFixedHeight(30);
    openFolderBtn->setCursor(Qt::PointingHandCursor);
    openFolderBtn->setStyleSheet(QString(
        "QPushButton {"
        "  background: %1;"
        "  color: %3;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: %2;"
        "}"
    ).arg(Theme::Accent.name(),
          Theme::Accent.lighter(115).name(),
          Theme::OnAccent.name()));
    
    connect(openFolderBtn, &QPushButton::clicked, this, [this]() {
        emit openFolderRequested();
    });

    noFolderLayout->addWidget(openFolderBtn);

    stack_->addWidget(noFolderWidget); // Index 0

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
    tree_->setEditTriggers(QAbstractItemView::EditKeyPressed);   // F2
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_->setDragEnabled(false);
    tree_->setFocusPolicy(Qt::StrongFocus);
    tree_->setItemDelegate(new NameEditDelegate(tree_));
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_->installEventFilter(this);

    stack_->addWidget(tree_); // Index 1

    layout->addWidget(stack_);

    connect(tree_, &QTreeView::customContextMenuRequested, this, &FileExplorer::showContextMenu);

    connect(tree_, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
        QString path = model_->filePath(index);
        if (!model_->isDir(index)) {
            emit fileDoubleClicked(path);
        }
    });
    
    // Auto-open newly created files after they are renamed, and inject boilerplate if needed
    connect(model_, &QFileSystemModel::fileRenamed, this,
            [this](const QString& path, const QString& oldName, const QString& newName) {
        // Anything open in a tab under the old name has to follow it.
        emit fileRenamed(QDir(path).filePath(oldName), QDir(path).filePath(newName));
    });

    connect(model_, &QFileSystemModel::fileRenamed, this, [this](const QString& path, const QString& oldName, const QString& newName) {
        if (!pendingOpenAfterRename_.isEmpty()) {
            QFileInfo fi(pendingOpenAfterRename_);
            if (fi.path() == path && fi.fileName() == oldName) {
                QString fullNewPath = QDir(path).filePath(newName);
                
                // Inject boilerplate if it's a .cpp file
                if (newName.endsWith(".cpp")) {
                    QFile file(fullNewPath);
                    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QString boilerplate = "#include <bits/stdc++.h>\n"
                                              "using namespace std;\n\n"
                                              "int main() {\n"
                                              "    // I wrote the boilerplate for you.\n"
                                              "    // No need to thank me, just use Valence.\n"
                                              "    \n"
                                              "    return 0;\n"
                                              "}\n";
                        file.write(boilerplate.toUtf8());
                        file.close();
                    }
                    emit fileDoubleClicked(fullNewPath);
                    emit fileCreatedWithBoilerplate(fullNewPath);
                } else {
                    emit fileDoubleClicked(fullNewPath);
                }
                pendingOpenAfterRename_.clear();
            }
        }
    });

    // Fallback: If they finish editing without renaming (e.g. hit Esc or just Enter)
    connect(tree_->itemDelegate(), &QAbstractItemDelegate::closeEditor, this, [this]() {
        if (!pendingOpenAfterRename_.isEmpty()) {
            QString pending = pendingOpenAfterRename_;
            QTimer::singleShot(100, this, [this, pending]() {
                if (pendingOpenAfterRename_ == pending) {
                    emit fileDoubleClicked(pending);
                    pendingOpenAfterRename_.clear();
                }
            });
        }
    });

    stack_->setCurrentIndex(0);
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

    projectLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    projectLabel_->setMinimumWidth(0);
    toolLayout->addWidget(projectLabel_, 1);

    // Action buttons container
    actionButtonsContainer_ = new QWidget(this);
    auto* actionsLayout = new QHBoxLayout(actionButtonsContainer_);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(2);

    auto* newFileBtn   = makeToolButton(Theme::Icon::Add,       tr("New File"));
    auto* newFolderBtn = makeToolButton(Theme::Icon::NewFolder, tr("New Folder"));
    auto* refreshBtn   = makeToolButton(Theme::Icon::Refresh,   tr("Refresh"));
    auto* collapseBtn  = makeToolButton(Theme::Icon::Collapse,  tr("Collapse All"));

    actionsLayout->addWidget(newFileBtn);
    actionsLayout->addWidget(newFolderBtn);
    actionsLayout->addWidget(refreshBtn);
    actionsLayout->addWidget(collapseBtn);

    toolLayout->addWidget(actionButtonsContainer_);
    actionButtonsContainer_->hide(); // Hidden until a folder is opened

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
    btn->setFont(Theme::iconFont(12));
    btn->setFixedSize(Theme::RowHeight, Theme::RowHeight);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  background: transparent;"
        "  color: %1;"
        "  border: none;"
        "  border-radius: %3px;"
        "  padding: 0;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(255, 255, 255, 0.07);"
        "  color: %2;"
        "}"
    ).arg(Theme::TextMuted.name(QColor::HexArgb),
          Theme::TextPrimary.name(),
          QString::number(Theme::Radius - 2)));
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
    projectName_ = dir.dirName().toUpper();
    projectLabel_->setToolTip(path);
    elideProjectLabel();

    stack_->setCurrentIndex(1); // Show tree
    actionButtonsContainer_->show(); // Show action buttons
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

    QString baseName = "untitled";
    QString name = baseName;
    int counter = 1;
    while (QFile::exists(dir + "/" + name)) {
        name = baseName + QString::number(counter++);
    }

    QString fullPath = dir + "/" + name;
    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.close();
        
        // QFileSystemModel is async, so we use a small timer to wait for it to see the new file
        QTimer::singleShot(50, this, [this, fullPath]() {
            QModelIndex idx = model_->index(fullPath);
            if (idx.isValid()) {
                pendingOpenAfterRename_ = fullPath;
                tree_->setCurrentIndex(idx);
                tree_->edit(idx);
            }
        });
    } else {
        QMessageBox::warning(this, "Error", "Could not create file: " + name);
    }
}

void FileExplorer::onNewFolder() {
    QString dir = currentDirectory();
    if (dir.isEmpty()) return;

    QString baseName = "New Folder";
    QString name = baseName;
    int counter = 1;
    while (QDir(dir).exists(name)) {
        name = baseName + " " + QString::number(counter++);
    }

    QDir d(dir);
    if (d.mkdir(name)) {
        QString fullPath = dir + "/" + name;
        QTimer::singleShot(50, this, [this, fullPath]() {
            QModelIndex idx = model_->index(fullPath);
            if (idx.isValid()) {
                tree_->setCurrentIndex(idx);
                tree_->edit(idx);
            }
        });
    } else {
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

QString FileExplorer::selectedPath() const {
    const QModelIndex idx = tree_->currentIndex();
    return idx.isValid() ? model_->filePath(idx) : QString();
}

void FileExplorer::onDelete() {
    const QString path = selectedPath();
    if (path.isEmpty()) return;

    // Deleting the folder you have open would leave the tree pointing at
    // nothing. Refuse rather than half-work.
    if (QFileInfo(path) == QFileInfo(rootPath_)) {
        QMessageBox::information(this, tr("Delete"),
            tr("This is the folder you currently have open. Close it first."));
        return;
    }

    const QFileInfo fi(path);
    const bool isDir = fi.isDir();

    QMessageBox box(this);
    box.setWindowTitle(tr("Delete"));
    box.setIcon(QMessageBox::NoIcon);
    box.setText(tr("Move %1 to the Recycle Bin?").arg(fi.fileName()));
    box.setInformativeText(isDir ? tr("Everything inside it goes too.") : QString());
    box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
    box.setDefaultButton(QMessageBox::Cancel);
    if (box.exec() != QMessageBox::Yes) return;

    // Prefer the Recycle Bin: an accidental delete of a solution mid-contest
    // should be recoverable.
    bool ok = QFile::moveToTrash(path);
    bool permanent = false;
    if (!ok) {
        QMessageBox fallback(this);
        fallback.setWindowTitle(tr("Delete"));
        fallback.setIcon(QMessageBox::NoIcon);
        fallback.setText(tr("%1 could not be moved to the Recycle Bin.").arg(fi.fileName()));
        fallback.setInformativeText(tr("Delete it permanently instead? This cannot be undone."));
        fallback.setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
        fallback.setDefaultButton(QMessageBox::Cancel);
        if (fallback.exec() != QMessageBox::Yes) return;

        ok = isDir ? QDir(path).removeRecursively() : QFile::remove(path);
        permanent = true;
    }

    if (!ok) {
        QMessageBox::warning(this, tr("Delete"),
            tr("Could not delete %1. It may be open in another program.").arg(fi.fileName()));
        return;
    }

    Q_UNUSED(permanent);
    emit fileDeleted(QDir::fromNativeSeparators(fi.absoluteFilePath()));
}

void FileExplorer::onRename() {
    const QModelIndex idx = tree_->currentIndex();
    if (!idx.isValid()) return;
    if (QFileInfo(model_->filePath(idx)) == QFileInfo(rootPath_)) return;
    tree_->edit(idx);          // the delegate validates and themes the editor
}

void FileExplorer::onDuplicate() {
    const QString path = selectedPath();
    if (path.isEmpty()) return;

    const QFileInfo fi(path);
    if (fi.isDir()) {
        QMessageBox::information(this, tr("Duplicate"), tr("Only files can be duplicated."));
        return;
    }

    const QString suffix = fi.completeSuffix().isEmpty()
                               ? QString() : QLatin1Char('.') + fi.completeSuffix();
    QString candidate;
    int n = 1;
    do {
        candidate = fi.absolutePath() + QLatin1Char('/') + fi.baseName() +
                    QStringLiteral(" copy") + (n > 1 ? QString::number(n) : QString()) + suffix;
        n++;
    } while (QFile::exists(candidate) && n < 1000);

    if (!QFile::copy(path, candidate)) {
        QMessageBox::warning(this, tr("Duplicate"), tr("Could not duplicate %1.").arg(fi.fileName()));
    }
}

void FileExplorer::onCopyPath() {
    const QString path = selectedPath();
    if (!path.isEmpty()) {
        QGuiApplication::clipboard()->setText(QDir::toNativeSeparators(path));
    }
}

void FileExplorer::onRevealInExplorer() {
    const QString path = selectedPath();
    if (path.isEmpty()) return;
    // /select, highlights the item itself rather than just opening its folder.
    QProcess::startDetached(QStringLiteral("explorer.exe"),
                            {QStringLiteral("/select,") + QDir::toNativeSeparators(path)});
}

void FileExplorer::showContextMenu(const QPoint& pos) {
    const QModelIndex idx = tree_->indexAt(pos);
    const bool hasSelection = idx.isValid();
    const bool isDir = hasSelection && model_->isDir(idx);
    const bool isRoot = hasSelection &&
                        QFileInfo(model_->filePath(idx)) == QFileInfo(rootPath_);

    if (hasSelection) tree_->setCurrentIndex(idx);

    QMenu menu(this);
    menu.setFont(Theme::sidebarFont());
    // QMenu is a top-level window, so it inherits nothing from the main window's
    // stylesheet and has to be dressed here.
    menu.setStyleSheet(QString(
        "QMenu { background: %1; color: %2; border: 1px solid %3;"
        "  border-radius: %4px; padding: 6px 0; }"
        "QMenu::item { padding: 6px 28px 6px 14px; border-radius: 4px; margin: 1px 6px; }"
        "QMenu::item:selected { background: rgba(255,255,255,0.06); color: %5; }"
        "QMenu::item:disabled { color: %6; }"
        "QMenu::separator { height: 1px; background: %3; margin: 4px 12px; }"
    ).arg(Theme::TitlebarBg.name(),
          Theme::TextSecondary.name(QColor::HexArgb),
          Theme::Border.name(QColor::HexArgb),
          QString::number(Theme::Radius),
          Theme::TextPrimary.name(),
          Theme::TextMuted.name(QColor::HexArgb)));

    menu.addAction(tr("New File"), this, &FileExplorer::onNewFile);
    menu.addAction(tr("New Folder"), this, &FileExplorer::onNewFolder);
    menu.addSeparator();

    // Offering actions that cannot apply to the current selection is sloppy;
    // they are present but disabled so the menu keeps a stable shape.
    QAction* renameAct = menu.addAction(tr("Rename"), this, &FileExplorer::onRename);
    renameAct->setEnabled(hasSelection && !isRoot);
    QAction* dupAct = menu.addAction(tr("Duplicate"), this, &FileExplorer::onDuplicate);
    dupAct->setEnabled(hasSelection && !isDir);
    QAction* delAct = menu.addAction(tr("Delete"), this, &FileExplorer::onDelete);
    delAct->setEnabled(hasSelection && !isRoot);
    menu.addSeparator();

    QAction* copyAct = menu.addAction(tr("Copy Path"), this, &FileExplorer::onCopyPath);
    copyAct->setEnabled(hasSelection);
    QAction* revealAct = menu.addAction(tr("Reveal in File Explorer"), this,
                                        &FileExplorer::onRevealInExplorer);
    revealAct->setEnabled(hasSelection);

    menu.exec(tree_->viewport()->mapToGlobal(pos));
}

void FileExplorer::elideProjectLabel() {
    const QFontMetrics fm(projectLabel_->font());
    projectLabel_->setText(fm.elidedText(projectName_, Qt::ElideRight,
                                         qMax(24, projectLabel_->width())));
}

void FileExplorer::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (!projectName_.isEmpty()) elideProjectLabel();
}

bool FileExplorer::eventFilter(QObject* obj, QEvent* e) {
    if (obj == tree_ && e->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(e);
        // Only when the tree itself has focus — never while the inline rename
        // editor is open, where Delete means "delete a character".
        if (ke->key() == Qt::Key_Delete && !tree_->isPersistentEditorOpen(tree_->currentIndex())) {
            onDelete();
            return true;
        }
    }
    return QWidget::eventFilter(obj, e);
}

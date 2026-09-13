#pragma once
#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>

class FileExplorer : public QWidget {
    Q_OBJECT

public:
    explicit FileExplorer(QWidget* parent = nullptr);
    void setRootPath(const QString& path);
    QString rootPath() const;

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

signals:
    void fileDoubleClicked(const QString& filePath);
    void fileCreatedWithBoilerplate(const QString& filePath);
    void newFileRequested(const QString& dirPath);
    void openFolderRequested();

    // Renaming or deleting a file that is open in a tab has to be reflected
    // there; the explorer cannot reach the tabs itself, so it reports instead.
    void fileDeleted(const QString& path);
    void fileRenamed(const QString& oldPath, const QString& newPath);

private slots:
    void onNewFile();
    void onNewFolder();
    void onRefresh();
    void onCollapseAll();
    void onDelete();
    void onRename();
    void onDuplicate();
    void onCopyPath();
    void onRevealInExplorer();
    void showContextMenu(const QPoint& pos);

private:
    QStackedWidget* stack_;
    QTreeView* tree_;
    QFileSystemModel* model_;
    QLabel* projectLabel_;
    QWidget* actionButtonsContainer_;
    QString rootPath_;
    QString projectName_;
    QString pendingOpenAfterRename_;

    void applyStyle();
    void elideProjectLabel();
    QString currentDirectory() const;
    QString selectedPath() const;
    QWidget* createToolbar();
    QPushButton* makeToolButton(const QString& text, const QString& tooltip);
};

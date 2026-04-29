#pragma once
#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QPushButton>
#include <QLabel>

class FileExplorer : public QWidget {
    Q_OBJECT

public:
    explicit FileExplorer(QWidget* parent = nullptr);
    void setRootPath(const QString& path);
    QString rootPath() const;

signals:
    void fileDoubleClicked(const QString& filePath);
    void newFileRequested(const QString& dirPath);

private slots:
    void onNewFile();
    void onNewFolder();
    void onRefresh();
    void onCollapseAll();

private:
    QTreeView* tree_;
    QFileSystemModel* model_;
    QLabel* projectLabel_;
    QString rootPath_;

    void applyStyle();
    QString currentDirectory() const;
    QWidget* createToolbar();
    QPushButton* makeToolButton(const QString& text, const QString& tooltip);
};

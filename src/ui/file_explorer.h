#pragma once
#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>

class FileExplorer : public QWidget {
    Q_OBJECT

public:
    explicit FileExplorer(QWidget* parent = nullptr);
    void setRootPath(const QString& path);
    QString rootPath() const;

signals:
    void fileDoubleClicked(const QString& filePath);

private:
    QTreeView* tree_;
    QFileSystemModel* model_;
    void applyStyle();
};

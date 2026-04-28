#pragma once
#include <QWidget>
#include <QTabWidget>
#include <QTabBar>
#include "../editor/editor_widget.h"

class TabWidget : public QWidget {
    Q_OBJECT

public:
    explicit TabWidget(QWidget* parent = nullptr);

    int addEditor(EditorWidget* editor, const QString& label);
    EditorWidget* currentEditor() const;
    EditorWidget* editorAt(int index) const;
    int findByFilePath(const QString& path) const;
    void setCurrentIndex(int index);
    int currentIndex() const;
    int count() const;
    void updateTabLabel(int index);
    void closeTab(int index);

signals:
    void currentChanged(int index);
    void tabCloseRequested(int index);

private:
    QTabWidget* tabs_;
    void applyStyle();
};

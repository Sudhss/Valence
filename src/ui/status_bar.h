#pragma once
#include <QWidget>
#include <QLabel>

class StatusBar : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent = nullptr);
    void setFileName(const QString& name);
    void setCursorPosition(int row, int col);
    void setLanguage(const QString& lang);
    void setEncoding(const QString& enc);

private:
    QLabel* fileLabel_;
    QLabel* posLabel_;
    QLabel* langLabel_;
    QLabel* encLabel_;
};

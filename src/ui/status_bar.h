#pragma once
#include <QWidget>
#include <QLabel>
#include <QTimer>

class StatusBar : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent = nullptr);
    void setFileName(const QString& name);
    void setCursorPosition(int row, int col);
    void setLanguage(const QString& lang);
    void setEncoding(const QString& enc);

public slots:
    // Transient progress from the judge panel. Clears itself so a stale
    // "Running test 3..." cannot sit there after the run has finished.
    void setMessage(const QString& text);

private:
    QLabel* fileLabel_;
    QLabel* posLabel_;
    QLabel* langLabel_;
    QLabel* encLabel_;
    QLabel* msgLabel_;
    QTimer* msgTimer_;
};

#include "terminal_widget.h"
#include "../theme/theme.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QRegularExpression>

TerminalWidget::TerminalWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header with tabs
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(12, 0, 12, 0);
    headerLayout->setSpacing(16);

    auto* problemsTab = new QLabel("PROBLEMS");
    auto* outputTab = new QLabel("OUTPUT");
    auto* terminalTab = new QLabel("TERMINAL");

    for (auto* label : {problemsTab, outputTab}) {
        label->setFont(Theme::statusFont());
        label->setFixedHeight(32);
        label->setStyleSheet(QString("color: %1; padding: 0 4px;")
            .arg(Theme::TextMuted.name(QColor::HexArgb)));
    }
    terminalTab->setFont(Theme::statusFont());
    terminalTab->setFixedHeight(32);
    terminalTab->setStyleSheet(QString(
        "color: %1; padding: 0 4px; border-bottom: 2px solid %2;"
    ).arg(Theme::TextPrimary.name(), Theme::Accent.name()));

    headerLayout->addWidget(problemsTab);
    headerLayout->addWidget(outputTab);
    headerLayout->addWidget(terminalTab);
    headerLayout->addStretch();

    auto* headerWidget = new QWidget();
    headerWidget->setLayout(headerLayout);
    headerWidget->setFixedHeight(32);
    headerWidget->setStyleSheet(QString(
        "background: %1; border-top: 1px solid %2;"
    ).arg(Theme::TerminalBg.name(), Theme::Border.name(QColor::HexArgb)));
    layout->addWidget(headerWidget);

    // Output area (now integrated terminal)
    output_ = new QPlainTextEdit(this);
    output_->setFont(Theme::terminalFont());
    output_->setMaximumBlockCount(5000);
    output_->installEventFilter(this);
    layout->addWidget(output_);

    // Start PowerShell process
    process_ = new QProcess(this);
    process_->setProcessChannelMode(QProcess::MergedChannels);
    connect(process_, &QProcess::readyRead, this, &TerminalWidget::onReadyRead);

    process_->start("powershell.exe", QStringList() << "-NoLogo" << "-NoProfile");

    applyStyle();
}

TerminalWidget::~TerminalWidget() {
    if (process_->state() != QProcess::NotRunning) {
        process_->kill();
        process_->waitForFinished(1000);
    }
}

void TerminalWidget::applyStyle() {
    output_->setStyleSheet(QString(
        "QPlainTextEdit {"
        "  background: %1;"
        "  color: %2;"
        "  border: none;"
        "  padding: 10px 14px;"
        "  selection-background-color: rgba(0, 255, 156, 0.12);"
        "}"
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
    ).arg(Theme::TerminalBg.name(), Theme::TextPrimary.name()));

    setStyleSheet(QString("background: %1;").arg(Theme::TerminalBg.name()));
}

void TerminalWidget::onReadyRead() {
    QByteArray data = process_->readAll();
    QString text = QString::fromLocal8Bit(data);
    // Strip basic ANSI escape sequences for clean output
    text.remove(QRegularExpression("\x1b\\[[0-9;]*[a-zA-Z]"));
    
    QTextCursor cursor = output_->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
    output_->setTextCursor(cursor);
    
    inputStartPosition_ = cursor.position();
}

void TerminalWidget::runCommand(const QString& cmd) {
    if (process_->state() == QProcess::Running) {
        QTextCursor cursor = output_->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(cmd + "\n");
        output_->setTextCursor(cursor);
        
        inputStartPosition_ = cursor.position();
        process_->write((cmd + "\n").toLocal8Bit());
    }
}

void TerminalWidget::clearOutput() {
    output_->clear();
    inputStartPosition_ = 0;
}

#include <QKeyEvent>
#include <QScrollBar>

bool TerminalWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == output_ && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        QTextCursor cursor = output_->textCursor();
        
        // If cursor is before the input start position, force it to the end before they type
        if (cursor.position() < inputStartPosition_) {
            if (!keyEvent->text().isEmpty() && keyEvent->key() != Qt::Key_Control && keyEvent->key() != Qt::Key_Shift) {
                cursor.movePosition(QTextCursor::End);
                output_->setTextCursor(cursor);
            }
        }

        switch (keyEvent->key()) {
        case Qt::Key_Backspace:
            // Don't allow deleting into the read-only output
            if (cursor.position() <= inputStartPosition_) return true;
            if (cursor.hasSelection() && cursor.selectionStart() < inputStartPosition_) return true;
            break;
            
        case Qt::Key_Left:
        case Qt::Key_Home:
            // Prevent navigating past the prompt with keys
            if (cursor.position() <= inputStartPosition_) {
                if (keyEvent->key() == Qt::Key_Left) return true;
                if (keyEvent->key() == Qt::Key_Home) {
                    cursor.setPosition(inputStartPosition_);
                    output_->setTextCursor(cursor);
                    return true;
                }
            }
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter: {
            cursor.movePosition(QTextCursor::End);
            output_->setTextCursor(cursor);
            
            // Extract the user's input
            cursor.setPosition(inputStartPosition_, QTextCursor::KeepAnchor);
            QString cmd = cursor.selectedText();
            cmd.replace(QChar::ParagraphSeparator, '\n');
            cursor.clearSelection();
            
            // Move cursor back to the very end before inserting the newline!
            cursor.movePosition(QTextCursor::End);
            
            // Insert newline manually and update input position
            cursor.insertText("\n");
            output_->setTextCursor(cursor);
            inputStartPosition_ = cursor.position();
            
            if (process_->state() == QProcess::Running) {
                process_->write((cmd + "\n").toLocal8Bit());
            }
            return true;
        }
        }
        
        // Always scroll to bottom if they interact
        output_->verticalScrollBar()->setValue(output_->verticalScrollBar()->maximum());
    }
    return QWidget::eventFilter(obj, event);
}

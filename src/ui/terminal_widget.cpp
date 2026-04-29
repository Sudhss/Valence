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

    // Output area
    output_ = new QPlainTextEdit(this);
    output_->setReadOnly(true);
    output_->setFont(Theme::terminalFont());
    output_->setMaximumBlockCount(5000);
    layout->addWidget(output_);

    // Input line
    input_ = new QLineEdit(this);
    input_->setFont(Theme::terminalFont());
    input_->setPlaceholderText("Type a command...");
    layout->addWidget(input_);

    connect(input_, &QLineEdit::returnPressed, this, &TerminalWidget::onCommandEntered);

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

    input_->setStyleSheet(QString(
        "QLineEdit {"
        "  background: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-top: 1px solid %3;"
        "  padding: 8px 14px;"
        "  font-size: 12px;"
        "}"
        "QLineEdit:focus {"
        "  border-top: 1px solid %4;"
        "}"
        "QLineEdit::placeholder {"
        "  color: %5;"
        "}"
    ).arg(Theme::TerminalBg.name(),
          Theme::TextPrimary.name(),
          Theme::Border.name(QColor::HexArgb),
          Theme::AccentDim.name(QColor::HexArgb),
          Theme::TextMuted.name(QColor::HexArgb)));

    setStyleSheet(QString("background: %1;").arg(Theme::TerminalBg.name()));
}

void TerminalWidget::onReadyRead() {
    QByteArray data = process_->readAll();
    QString text = QString::fromLocal8Bit(data);
    // Strip basic ANSI escape sequences for clean output
    text.remove(QRegularExpression("\x1b\\[[0-9;]*[a-zA-Z]"));
    output_->appendPlainText(text);
    // Auto-scroll to bottom
    auto cursor = output_->textCursor();
    cursor.movePosition(QTextCursor::End);
    output_->setTextCursor(cursor);
}

void TerminalWidget::onCommandEntered() {
    QString cmd = input_->text();
    if (cmd.isEmpty()) return;

    output_->appendPlainText("> " + cmd);
    input_->clear();

    if (process_->state() == QProcess::Running) {
        process_->write((cmd + "\n").toLocal8Bit());
    }
}

void TerminalWidget::runCommand(const QString& cmd) {
    if (process_->state() == QProcess::Running) {
        output_->appendPlainText("> " + cmd);
        process_->write((cmd + "\n").toLocal8Bit());
    }
}

void TerminalWidget::clearOutput() {
    output_->clear();
}

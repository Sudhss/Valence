#include "terminal_widget.h"
#include "../theme/theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QProcessEnvironment>

namespace {

/* Resolving the shell by bare name means trusting PATH, and PATH is not
 * something a shipped application gets to assume. powershell.exe does not live
 * in System32 itself but in a WindowsPowerShell/v1.0 subdirectory, so any
 * process launched with a trimmed environment — a scheduler, a service, a
 * parent that sanitised its own PATH — starts Valence with a terminal that
 * cannot open and no obvious reason why.
 *
 * So: look for the modern shell, then the classic one, then fall back to the
 * absolute location derived from the real system root, then cmd. */
QString resolveShell(QStringList& argsOut) {
    const QString pwsh = QStandardPaths::findExecutable(QStringLiteral("pwsh"));
    if (!pwsh.isEmpty()) {
        argsOut = {QStringLiteral("-NoLogo"), QStringLiteral("-NoProfile")};
        return pwsh;
    }

    QString ps = QStandardPaths::findExecutable(QStringLiteral("powershell"));
    if (ps.isEmpty()) {
        const QString root = QProcessEnvironment::systemEnvironment()
                                 .value(QStringLiteral("SystemRoot"), QStringLiteral("C:/Windows"));
        const QString candidate =
            QDir(root).filePath(QStringLiteral("System32/WindowsPowerShell/v1.0/powershell.exe"));
        if (QFileInfo::exists(candidate)) ps = candidate;
    }
    if (!ps.isEmpty()) {
        argsOut = {QStringLiteral("-NoLogo"), QStringLiteral("-NoProfile")};
        return ps;
    }

    const QString cmd = QStandardPaths::findExecutable(QStringLiteral("cmd"));
    if (!cmd.isEmpty()) { argsOut = {}; return cmd; }
    return QString();
}

} // namespace

TerminalWidget::TerminalWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ── Header ──
    // This used to show PROBLEMS / OUTPUT / TERMINAL, but only TERMINAL did
    // anything: the other two never responded to a click. Inert controls read
    // as an unfinished prototype, so they are gone rather than faked.
    auto* headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(Theme::HeaderHeight);
    auto* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(Theme::SpaceM, 0, Theme::SpaceS, 0);
    headerLayout->setSpacing(Theme::SpaceS);

    auto* terminalTab = new QLabel(tr("TERMINAL"), headerWidget);
    terminalTab->setFont(Theme::statusFont());
    terminalTab->setStyleSheet(QString("color: %1; letter-spacing: 1px;")
                                   .arg(Theme::TextPrimary.name()));

    statusLabel_ = new QLabel(headerWidget);
    statusLabel_->setFont(Theme::statusFont());
    statusLabel_->setStyleSheet(QString("color: %1;").arg(Theme::TextMuted.name(QColor::HexArgb)));

    auto* clearButton = new QPushButton(Theme::Icon::Clear, headerWidget);
    clearButton->setFont(Theme::iconFont(12));
    clearButton->setFixedSize(Theme::RowHeight, Theme::RowHeight);
    clearButton->setCursor(Qt::PointingHandCursor);
    clearButton->setToolTip(tr("Clear  (Ctrl+L)"));
    clearButton->setFocusPolicy(Qt::NoFocus);
    clearButton->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: none;"
        "  border-radius: %2px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.07); color: %3; }"
    ).arg(Theme::TextMuted.name(QColor::HexArgb),
          QString::number(Theme::Radius - 2),
          Theme::TextPrimary.name()));
    connect(clearButton, &QPushButton::clicked, this, &TerminalWidget::clearOutput);

    headerLayout->addWidget(terminalTab);
    headerLayout->addWidget(statusLabel_);
    headerLayout->addStretch();
    headerLayout->addWidget(clearButton);

    headerWidget->setStyleSheet(QString(
        "background: %1; border-top: 1px solid %2; border-bottom: 1px solid %3;")
            .arg(Theme::chromeGradient(Theme::Chrome, Theme::Base),
                 Theme::HighlightTop.name(QColor::HexArgb),
                 Theme::Border.name(QColor::HexArgb)));
    layout->addWidget(headerWidget);

    // ── Output / input surface ──
    output_ = new QPlainTextEdit(this);
    output_->setFont(Theme::terminalFont());
    output_->setMaximumBlockCount(5000);
    output_->setFrameShape(QFrame::NoFrame);
    output_->installEventFilter(this);
    layout->addWidget(output_);

    applyStyle();
    startShell();
}

TerminalWidget::~TerminalWidget() {
    if (process_) {
        process_->disconnect(this);
        if (process_->state() != QProcess::NotRunning) {
            process_->kill();
            process_->waitForFinished(1000);
        }
    }
}

void TerminalWidget::startShell() {
    if (process_) {
        process_->disconnect(this);
        process_->deleteLater();
        process_ = nullptr;
    }

    process_ = new QProcess(this);
    process_->setProcessChannelMode(QProcess::MergedChannels);
    connect(process_, &QProcess::readyRead, this, &TerminalWidget::onReadyRead);

    connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        if (err != QProcess::FailedToStart) return;
        // Previously this left the user staring at an empty black box with no
        // explanation of why nothing happened.
        shellAlive_ = false;
        statusLabel_->setText(tr("· unavailable"));
        appendOutput(tr("\nCould not start %1.\n")
                         .arg(QFileInfo(process_->program()).fileName()),
                     Theme::Failure);
    });

    connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus) {
        shellAlive_ = false;
        statusLabel_->setText(tr("· exited"));
        appendOutput(tr("\n[process exited with code %1 — press Enter to restart]\n").arg(code),
                     Theme::TextMuted);
        markInputStart();
    });

    QStringList args;
    const QString shell = resolveShell(args);
    if (shell.isEmpty()) {
        shellAlive_ = false;
        statusLabel_->setText(tr("- unavailable"));
        appendOutput(tr("\nNo shell could be found. Looked for pwsh, powershell and cmd.\n"),
                     Theme::Failure);
        return;
    }

    shellAlive_ = true;
    statusLabel_->clear();
    ansiPending_.clear();
    if (!workingDir_.isEmpty() && QDir(workingDir_).exists()) {
        process_->setWorkingDirectory(workingDir_);
    }
    process_->start(shell, args);
}

void TerminalWidget::setWorkingDirectory(const QString& dir) {
    if (dir.isEmpty() || dir == workingDir_ || !QDir(dir).exists()) return;
    workingDir_ = dir;

    // A live shell cannot have its directory changed from outside, so move it
    // there. Written straight to stdin rather than echoed into the view: the
    // shell's own new prompt is the feedback, and a stream of "cd" lines the
    // user never typed would just be noise.
    if (process_ && shellAlive_ && process_->state() == QProcess::Running) {
        const QString cd = QStringLiteral("cd \"%1\"\n")
                               .arg(QDir::toNativeSeparators(dir));
        process_->write(cd.toLocal8Bit());
    }
}

void TerminalWidget::applyStyle() {
    output_->setStyleSheet(QString(
        "QPlainTextEdit {"
        "  background: %1;"
        "  color: %2;"
        "  border: none;"
        "  padding: 10px 14px;"
        "  selection-background-color: %5;"
        "  selection-color: %2;"
        "}"
        "QScrollBar:vertical { width: 6px; background: transparent; margin: 0; }"
        "QScrollBar::handle:vertical { background: %3; border-radius: 3px; min-height: 24px; }"
        "QScrollBar::handle:vertical:hover { background: %4; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        "QScrollBar:horizontal { height: 6px; background: transparent; margin: 0; }"
        "QScrollBar::handle:horizontal { background: %3; border-radius: 3px; min-width: 24px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }"
    ).arg(Theme::Base.name(),
          Theme::TextPrimary.name(),
          Theme::ScrollThumb.name(QColor::HexArgb),
          Theme::ScrollThumbHover.name(QColor::HexArgb),
          Theme::SelectionBg.name(QColor::HexArgb)));

    setStyleSheet(QString("background: %1;").arg(Theme::TerminalBg.name()));
}

// ── Escape-sequence handling ─────────────────────────────────────────────────

QString TerminalWidget::stripAnsi(const QString& chunk) {
    // A sequence can be split across two reads, so anything incomplete at the
    // end of a chunk is carried over rather than rendered as garbage.
    QString text = ansiPending_ + chunk;
    ansiPending_.clear();

    QString out;
    out.reserve(text.size());

    int i = 0;
    while (i < text.size()) {
        const QChar c = text.at(i);
        if (c != QLatin1Char('\x1b')) {
            if (c != QLatin1Char('\r')) out.append(c);   // CR would double-space every line
            i++;
            continue;
        }

        if (i + 1 >= text.size()) { ansiPending_ = text.mid(i); break; }
        const QChar kind = text.at(i + 1);

        if (kind == QLatin1Char('[')) {                  // CSI: ESC [ params final
            int j = i + 2;
            while (j < text.size() && !text.at(j).isLetter()) j++;
            if (j >= text.size()) { ansiPending_ = text.mid(i); break; }
            i = j + 1;
        } else if (kind == QLatin1Char(']')) {           // OSC, terminated by BEL or ST
            int j = i + 2;
            while (j < text.size()) {
                if (text.at(j) == QLatin1Char('\a')) { j++; break; }
                if (text.at(j) == QLatin1Char('\x1b') && j + 1 < text.size() &&
                    text.at(j + 1) == QLatin1Char('\\')) { j += 2; break; }
                j++;
            }
            if (j >= text.size() && !text.endsWith(QLatin1Char('\a'))) {
                ansiPending_ = text.mid(i);
                break;
            }
            i = j;
        } else {
            i += 2;                                      // two-character escape
        }
    }
    return out;
}

// ── Output ───────────────────────────────────────────────────────────────────

bool TerminalWidget::atBottom() const {
    QScrollBar* bar = output_->verticalScrollBar();
    return bar->value() >= bar->maximum() - 4;
}

void TerminalWidget::appendOutput(const QString& text, const QColor& color) {
    if (text.isEmpty()) return;

    // Preserve where the user was looking and what they had selected. Forcing
    // the caret to the end on every chunk fought anyone reading back through a
    // build log.
    const bool follow = atBottom();
    QTextCursor saved = output_->textCursor();
    const bool hadSelection = saved.hasSelection();

    QTextCursor cursor(output_->document());
    cursor.movePosition(QTextCursor::End);
    if (color.isValid()) {
        QTextCharFormat fmt;
        fmt.setForeground(color);
        cursor.insertText(text, fmt);
    } else {
        cursor.insertText(text, QTextCharFormat());
    }

    if (hadSelection) {
        output_->setTextCursor(saved);
    } else if (follow) {
        QTextCursor end(output_->document());
        end.movePosition(QTextCursor::End);
        output_->setTextCursor(end);
    }
    if (follow) {
        output_->verticalScrollBar()->setValue(output_->verticalScrollBar()->maximum());
    }
}

void TerminalWidget::onReadyRead() {
    if (!process_) return;
    const QString text = stripAnsi(QString::fromLocal8Bit(process_->readAll()));
    if (text.isEmpty()) return;
    appendOutput(text);
    markInputStart();
}

void TerminalWidget::markInputStart() {
    // Relative to the last block, so trimming old lines cannot invalidate it.
    inputColumn_ = output_->document()->lastBlock().text().length();
}

int TerminalWidget::inputStart() const {
    const QTextBlock last = output_->document()->lastBlock();
    return last.position() + qMin(inputColumn_, last.text().length());
}

QString TerminalWidget::currentInput() const {
    QTextCursor c(output_->document());
    c.setPosition(inputStart());
    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    QString s = c.selectedText();
    s.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    return s;
}

void TerminalWidget::replaceInput(const QString& text) {
    QTextCursor c(output_->document());
    c.setPosition(inputStart());
    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    c.insertText(text);
    output_->setTextCursor(c);
    output_->verticalScrollBar()->setValue(output_->verticalScrollBar()->maximum());
}

void TerminalWidget::submitInput() {
    const QString cmd = currentInput();

    QTextCursor end(output_->document());
    end.movePosition(QTextCursor::End);
    end.insertText(QStringLiteral("\n"));
    output_->setTextCursor(end);
    markInputStart();

    if (!shellAlive_) {
        startShell();                 // Enter on a dead shell brings it back
        return;
    }

    if (!cmd.trimmed().isEmpty()) {
        history_.removeAll(cmd);
        history_.append(cmd);
        if (history_.size() > 200) history_.removeFirst();
    }
    historyIndex_ = history_.size();
    historyDraft_.clear();

    process_->write((cmd + QLatin1Char('\n')).toLocal8Bit());
}

void TerminalWidget::runCommand(const QString& cmd) {
    if (!process_ || !shellAlive_) startShell();
    if (!process_ || process_->state() == QProcess::NotRunning) return;

    appendOutput(cmd + QLatin1Char('\n'));
    markInputStart();
    process_->write((cmd + QLatin1Char('\n')).toLocal8Bit());
}

void TerminalWidget::clearOutput() {
    output_->clear();
    inputColumn_ = 0;
}

// ── Input handling ───────────────────────────────────────────────────────────

bool TerminalWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj != output_ || event->type() != QEvent::KeyPress) {
        return QWidget::eventFilter(obj, event);
    }

    auto* keyEvent = static_cast<QKeyEvent*>(event);
    const bool ctrl = keyEvent->modifiers() & Qt::ControlModifier;
    QTextCursor cursor = output_->textCursor();
    const int start = inputStart();

    if (ctrl) {
        switch (keyEvent->key()) {
        case Qt::Key_C:
            // With a selection this is copy; without one it is the interrupt,
            // which is what Ctrl+C means in a terminal.
            if (cursor.hasSelection()) {
                QApplication::clipboard()->setText(cursor.selectedText());
            } else if (shellAlive_) {
                replaceInput(QString());
                appendOutput(QStringLiteral("^C\n"), Theme::TextMuted);
                markInputStart();
                process_->write("\x03");
            }
            return true;
        case Qt::Key_L:
            clearOutput();
            return true;
        case Qt::Key_V: {
            QString text = QApplication::clipboard()->text();
            text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
            // Paste always lands in the input region, even if the caret was
            // parked up in the output.
            if (cursor.position() < start) {
                cursor.movePosition(QTextCursor::End);
                output_->setTextCursor(cursor);
            }
            const int newline = text.indexOf(QLatin1Char('\n'));
            if (newline >= 0) {
                // Multi-line paste: run each complete line, keep the remainder.
                output_->textCursor().insertText(text.left(newline));
                submitInput();
                const QString rest = text.mid(newline + 1);
                if (!rest.isEmpty()) output_->textCursor().insertText(rest);
            } else {
                output_->textCursor().insertText(text);
            }
            return true;
        }
        default:
            break;
        }
        return QWidget::eventFilter(obj, event);   // Ctrl+A, Ctrl+C-with-selection etc.
    }

    switch (keyEvent->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down: {
        if (history_.isEmpty()) return true;
        if (historyIndex_ == history_.size()) historyDraft_ = currentInput();

        if (keyEvent->key() == Qt::Key_Up) {
            if (historyIndex_ > 0) historyIndex_--;
        } else {
            if (historyIndex_ < history_.size()) historyIndex_++;
        }
        replaceInput(historyIndex_ < history_.size() ? history_.at(historyIndex_)
                                                     : historyDraft_);
        return true;
    }

    case Qt::Key_Backspace:
        if (cursor.hasSelection()) {
            if (cursor.selectionStart() < start) return true;
            break;
        }
        if (cursor.position() <= start) return true;
        break;

    case Qt::Key_Delete:
        // Delete could eat committed output; Backspace was guarded but this
        // was not.
        if (cursor.hasSelection()) {
            if (cursor.selectionStart() < start) return true;
        } else if (cursor.position() < start) {
            return true;
        }
        break;

    case Qt::Key_Left:
        if (!cursor.hasSelection() && cursor.position() <= start) return true;
        break;

    case Qt::Key_Home:
        cursor.setPosition(start, (keyEvent->modifiers() & Qt::ShiftModifier)
                                      ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
        output_->setTextCursor(cursor);
        return true;

    case Qt::Key_Return:
    case Qt::Key_Enter:
        submitInput();
        return true;

    default:
        break;
    }

    // Any other text-producing key must land in the editable region, and must
    // never overwrite committed output via a selection that reaches into it.
    if (!keyEvent->text().isEmpty() && keyEvent->text().at(0).isPrint()) {
        if (cursor.hasSelection() && cursor.selectionStart() < start) {
            cursor.clearSelection();
            cursor.movePosition(QTextCursor::End);
            output_->setTextCursor(cursor);
        } else if (cursor.position() < start) {
            cursor.movePosition(QTextCursor::End);
            output_->setTextCursor(cursor);
        }
    }

    return QWidget::eventFilter(obj, event);
}

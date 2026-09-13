#include "test_case_card.h"
#include "../theme/theme.h"
#include "../cph/judge.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>

namespace {

QColor verdictColor(Verdict v) {
    switch (v) {
    case Verdict::Accepted:     return Theme::Success;
    case Verdict::WrongAnswer:  return Theme::Failure;
    case Verdict::RuntimeError: return Theme::Failure;
    case Verdict::CompileError: return Theme::Failure;
    case Verdict::TimeLimit:    return Theme::Warning;
    case Verdict::Running:      return Theme::Accent;
    case Verdict::Pending:      break;
    }
    return Theme::Pending;
}

QColor verdictFill(Verdict v) {
    switch (v) {
    case Verdict::Accepted:     return Theme::SuccessBg;
    case Verdict::WrongAnswer:
    case Verdict::RuntimeError:
    case Verdict::CompileError: return Theme::FailureBg;
    case Verdict::TimeLimit:    return Theme::WarningBg;
    case Verdict::Running:      return Theme::AccentDim;
    case Verdict::Pending:      break;
    }
    return Theme::PendingBg;
}

QString editorStyle() {
    return QString(
        "QPlainTextEdit {"
        "  background: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: %4px;"
        "  padding: 6px 8px;"
        "  selection-background-color: %8;"
        "  selection-color: %2;"
        "}"
        "QPlainTextEdit:focus { border: 1px solid %5; }"
        "QPlainTextEdit[readOnly=\"true\"] { color: %6; }"
        "QScrollBar:vertical { width: 4px; background: transparent; margin: 0; }"
        "QScrollBar::handle:vertical { background: %7; border-radius: 2px; min-height: 18px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        "QScrollBar:horizontal { height: 4px; background: transparent; margin: 0; }"
        "QScrollBar::handle:horizontal { background: %7; border-radius: 2px; min-width: 18px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }"
    ).arg(Theme::Base.name(),
          Theme::TextPrimary.name(),
          Theme::Border.name(QColor::HexArgb),
          QString::number(Theme::Radius - 2),
          Theme::AccentDim.name(QColor::HexArgb),
          Theme::TextSecondary.name(QColor::HexArgb),
          Theme::ScrollThumb.name(QColor::HexArgb),
          Theme::SelectionBg.name(QColor::HexArgb));
}

QLabel* fieldLabel(const QString& text) {
    auto* l = new QLabel(text);
    // Pixels, not points: the theme fonts are sized in pixels, and setting a
    // point size here would override that and come out larger, not smaller.
    l->setFont(Theme::uiFont(Theme::FontSizeSmall - 1));
    l->setStyleSheet(QString("color: %1; letter-spacing: 1px;")
                         .arg(Theme::TextMuted.name(QColor::HexArgb)));
    return l;
}

QPlainTextEdit* makeEditor(bool readOnly) {
    auto* e = new QPlainTextEdit;
    e->setFont(Theme::terminalFont());
    e->setReadOnly(readOnly);
    e->setLineWrapMode(QPlainTextEdit::NoWrap);
    e->setFrameShape(QFrame::NoFrame);
    e->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    e->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    e->setTabChangesFocus(true);
    e->setStyleSheet(editorStyle());
    return e;
}

} // namespace

TestCaseCard::TestCaseCard(int index, QWidget* parent)
    : QWidget(parent), index_(index) {
    setAttribute(Qt::WA_StyledBackground, false);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::SpaceS, Theme::SpaceS, Theme::SpaceS, Theme::SpaceS);
    root->setSpacing(Theme::SpaceS);

    // ── Header ──
    header_ = new QWidget(this);
    header_->setFixedHeight(Theme::RowHeight);
    header_->setCursor(Qt::PointingHandCursor);
    header_->installEventFilter(this);

    auto* hl = new QHBoxLayout(header_);
    hl->setContentsMargins(0, 0, 0, 0);
    hl->setSpacing(Theme::SpaceS);

    indexLabel_ = new QLabel(header_);
    indexLabel_->setFont(Theme::sidebarFont());
    indexLabel_->setStyleSheet(QString("color: %1;").arg(Theme::TextSecondary.name(QColor::HexArgb)));

    badge_ = new QLabel(header_);
    QFont badgeFont = Theme::statusFont();
    badgeFont.setBold(true);
    badge_->setFont(badgeFont);
    badge_->setAlignment(Qt::AlignCenter);
    badge_->setMinimumWidth(34);

    timeLabel_ = new QLabel(header_);
    timeLabel_->setFont(Theme::statusFont());
    timeLabel_->setStyleSheet(QString("color: %1;").arg(Theme::TextMuted.name(QColor::HexArgb)));

    removeButton_ = new QPushButton(Theme::Icon::Close, header_);
    removeButton_->setFont(Theme::iconFont(10));
    removeButton_->setFixedSize(18, 18);
    removeButton_->setCursor(Qt::PointingHandCursor);
    removeButton_->setToolTip(tr("Remove this test case"));
    removeButton_->setFocusPolicy(Qt::NoFocus);
    removeButton_->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: none;"
        "  border-radius: 9px; }"
        "QPushButton:hover { background: %2; color: %3; }"
    ).arg(Theme::TextMuted.name(QColor::HexArgb),
          Theme::FailureBg.name(QColor::HexArgb),
          Theme::Failure.name()));
    // Reserve the slot always, so the row never changes width on hover.
    removeButton_->setVisible(false);
    connect(removeButton_, &QPushButton::clicked, this, &TestCaseCard::removeRequested);

    hl->addWidget(indexLabel_);
    hl->addStretch();
    hl->addWidget(timeLabel_);
    hl->addWidget(badge_);
    hl->addWidget(removeButton_);
    root->addWidget(header_);

    // ── Body ──
    body_ = new QWidget(this);
    auto* bl = new QVBoxLayout(body_);
    bl->setContentsMargins(0, 0, 0, 0);
    bl->setSpacing(Theme::SpaceXS);

    bl->addWidget(fieldLabel(tr("INPUT")));
    input_ = makeEditor(false);
    bl->addWidget(input_);

    bl->addSpacing(Theme::SpaceXS);
    bl->addWidget(fieldLabel(tr("EXPECTED")));
    expected_ = makeEditor(false);
    bl->addWidget(expected_);

    actualBlock_ = new QWidget(body_);
    auto* al = new QVBoxLayout(actualBlock_);
    al->setContentsMargins(0, Theme::SpaceXS, 0, 0);
    al->setSpacing(Theme::SpaceXS);
    actualLabel_ = fieldLabel(tr("ACTUAL"));
    al->addWidget(actualLabel_);
    actual_ = makeEditor(true);
    al->addWidget(actual_);
    actualBlock_->setVisible(false);
    bl->addWidget(actualBlock_);

    root->addWidget(body_);

    // Editing invalidates the verdict: a green badge sitting above changed
    // input is a lie, not a cosmetic detail.
    auto onEdit = [this](QPlainTextEdit* e) {
        return [this, e]() {
            autoSize(e);
            if (verdict_ != Verdict::Pending) setVerdict(Verdict::Pending);
            timeLabel_->clear();
            actualBlock_->setVisible(false);
            emit edited();
        };
    };
    connect(input_, &QPlainTextEdit::textChanged, this, onEdit(input_));
    connect(expected_, &QPlainTextEdit::textChanged, this, onEdit(expected_));

    setIndex(index);
    refreshBadge();
    autoSize(input_);
    autoSize(expected_);
}

void TestCaseCard::setIndex(int index) {
    index_ = index;
    indexLabel_->setText(tr("Test %1").arg(index + 1));
}

TestCase TestCaseCard::data() const {
    TestCase tc;
    tc.input = input_->toPlainText();
    tc.expected = expected_->toPlainText();
    tc.verdict = verdict_;
    return tc;
}

void TestCaseCard::setData(const TestCase& tc) {
    QSignalBlocker b1(input_), b2(expected_);
    input_->setPlainText(tc.input);
    expected_->setPlainText(tc.expected);
    autoSize(input_);
    autoSize(expected_);
    setVerdict(tc.verdict);
}

void TestCaseCard::applyResult(const TestCase& result) {
    setVerdict(result.verdict);

    if (result.verdict == Verdict::Pending || result.verdict == Verdict::Running) {
        timeLabel_->clear();
    } else {
        timeLabel_->setText(tr("%1 ms").arg(result.elapsedMs));
    }

    const bool showActual = (result.verdict == Verdict::WrongAnswer ||
                             result.verdict == Verdict::RuntimeError ||
                             result.verdict == Verdict::TimeLimit);
    if (showActual) {
        QString text = result.actual;
        if (!result.errorText.isEmpty()) {
            if (!text.isEmpty() && !text.endsWith(QLatin1Char('\n'))) text.append(QLatin1Char('\n'));
            text += QStringLiteral("— ") + result.errorText.trimmed();
        }
        actual_->setPlainText(text);
        autoSize(actual_);
        actualLabel_->setText(result.verdict == Verdict::WrongAnswer ? tr("ACTUAL") : tr("OUTPUT"));
        actualBlock_->setVisible(true);
        if (!isExpanded()) setExpanded(true);   // a failure the user cannot see is useless

        // Land on the line that actually differs rather than the top of a long
        // output the user then has to scan by hand.
        const int line = Judge::firstDifferingLine(result.actual, result.expected);
        if (line > 0) {
            QTextCursor c = actual_->textCursor();
            c.movePosition(QTextCursor::Start);
            c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line);
            actual_->setTextCursor(c);
            actual_->centerCursor();
        }
    } else {
        actualBlock_->setVisible(false);
    }
    update();
}

void TestCaseCard::setVerdict(Verdict v) {
    verdict_ = v;
    refreshBadge();
    update();
}

void TestCaseCard::refreshBadge() {
    badge_->setText(verdictLabel(verdict_));
    badge_->setToolTip(verdictTooltip(verdict_));
    badge_->setStyleSheet(QString(
        "background: %1; color: %2; border-radius: %3px; padding: 2px 7px;"
    ).arg(verdictFill(verdict_).name(QColor::HexArgb),
          verdictColor(verdict_).name(),
          QString::number(Theme::Radius - 2)));
}

void TestCaseCard::setExpanded(bool on) {
    if (expanded_ == on) return;
    expanded_ = on;
    body_->setVisible(on);
    updateGeometry();
}

void TestCaseCard::autoSize(QPlainTextEdit* edit) {
    // Grow with the content so a two-line case does not occupy the same space
    // as a forty-line one, but never grow without bound.
    const int lines = qMax(1, edit->document()->blockCount());
    const QFontMetrics fm(edit->font());
    const int chrome = 16;                       // padding + border
    const int wanted = lines * fm.lineSpacing() + chrome;
    edit->setFixedHeight(qBound(fm.lineSpacing() + chrome, wanted, 220));
}

bool TestCaseCard::eventFilter(QObject* obj, QEvent* e) {
    if (obj == header_ && e->type() == QEvent::MouseButtonRelease) {
        auto* me = static_cast<QMouseEvent*>(e);
        if (me->button() == Qt::LeftButton) {
            setExpanded(!expanded_);
            return true;
        }
    }
    return QWidget::eventFilter(obj, e);
}

void TestCaseCard::enterEvent(QEnterEvent* e) {
    removeButton_->setVisible(true);
    QWidget::enterEvent(e);
}

void TestCaseCard::leaveEvent(QEvent* e) {
    removeButton_->setVisible(false);
    QWidget::leaveEvent(e);
}

void TestCaseCard::paintEvent(QPaintEvent* e) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(r, Theme::RadiusPanel, Theme::RadiusPanel);

    p.fillPath(path, Theme::Raised);

    // A failing card gets a tinted hairline, not a filled red block — the
    // badge already carries the verdict.
    QColor border = Theme::Border;
    if (verdict_ != Verdict::Pending) {
        border = verdictColor(verdict_);
        border.setAlpha(verdict_ == Verdict::Accepted ? 64 : 96);
    }
    p.setPen(QPen(border, 1));
    p.drawPath(path);

    // A one-pixel highlight along the top edge only. This is what separates a
    // card that sits ON the panel from a rectangle painted INTO it.
    p.setPen(QPen(Theme::HighlightTop, 1));
    p.drawLine(QPointF(r.left() + Theme::RadiusPanel, r.top()),
               QPointF(r.right() - Theme::RadiusPanel, r.top()));

    QWidget::paintEvent(e);
}

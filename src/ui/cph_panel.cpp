#include "cph_panel.h"
#include "test_case_card.h"
#include "../cph/judge.h"
#include "../theme/theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QPlainTextEdit>
#include <QShortcut>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <utility>

namespace {

QPushButton* makeHeaderButton(const QString& glyph, const QString& tip, const QColor& tint) {
    auto* b = new QPushButton(glyph);
    b->setFixedSize(22, 22);
    b->setCursor(Qt::PointingHandCursor);
    b->setToolTip(tip);
    b->setFocusPolicy(Qt::NoFocus);
    QColor hover = tint;
    hover.setAlpha(28);
    b->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: none;"
        "  border-radius: %2px; font-size: 12px; }"
        "QPushButton:hover { background: %3; color: %4; }"
        "QPushButton:disabled { color: %5; }"
    ).arg(Theme::TextSecondary.name(QColor::HexArgb),
          QString::number(Theme::Radius - 2),
          hover.name(QColor::HexArgb),
          tint.name(),
          Theme::TextMuted.name(QColor::HexArgb)));
    return b;
}

} // namespace

struct CphPanel::Impl {
    CphPanel* q = nullptr;

    QString target;
    QVector<TestCase> cases;
    QList<TestCaseCard*> cards;
    Judge* judge = nullptr;

    QLabel* summary = nullptr;
    QPushButton* runButton = nullptr;
    QPushButton* stopButton = nullptr;
    QPushButton* addButton = nullptr;

    QScrollArea* scroll = nullptr;
    QWidget* cardsHost = nullptr;
    QVBoxLayout* cardsLayout = nullptr;
    QLabel* emptyState = nullptr;

    QWidget* banner = nullptr;
    QPlainTextEdit* bannerText = nullptr;

    void addCard(const TestCase& tc, bool expanded);
    void clearCards();
    void renumber();
    void updateSummary();
    void setRunning(bool running);
    void showBanner(const QString& text);
    void collectFromCards();

    QString sidecarPath() const;
    void load();
    void save() const;
};

// ── Construction ─────────────────────────────────────────────────────────────

CphPanel::CphPanel(QWidget* parent) : QWidget(parent), d(new Impl) {
    d->q = this;

    setStyleSheet(QString("CphPanel { background: %1; }").arg(Theme::PanelBg.name()));
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(260);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Header ──
    auto* header = new QWidget(this);
    header->setFixedHeight(Theme::HeaderHeight);
    header->setStyleSheet(QString("background: %1; border-bottom: 1px solid %2;")
                              .arg(Theme::TerminalBg.name(),
                                   Theme::Border.name(QColor::HexArgb)));
    auto* hl = new QHBoxLayout(header);
    hl->setContentsMargins(Theme::SpaceM, 0, Theme::SpaceS, 0);
    hl->setSpacing(Theme::SpaceS);

    auto* title = new QLabel(tr("TESTS"), header);
    title->setFont(Theme::statusFont());
    title->setStyleSheet(QString("color: %1; letter-spacing: 1px;")
                             .arg(Theme::TextPrimary.name()));

    d->summary = new QLabel(header);
    d->summary->setFont(Theme::statusFont());
    d->summary->setAlignment(Qt::AlignCenter);

    d->addButton  = makeHeaderButton(QStringLiteral("+"), tr("Add a test case"), Theme::AccentBlue);
    d->runButton  = makeHeaderButton(QStringLiteral("▶"), tr("Run all tests  (Ctrl+Enter)"), Theme::Accent);
    d->stopButton = makeHeaderButton(QStringLiteral("■"), tr("Stop"), Theme::Failure);
    d->stopButton->setVisible(false);

    hl->addWidget(title);
    hl->addStretch();
    hl->addWidget(d->summary);
    hl->addWidget(d->addButton);
    hl->addWidget(d->runButton);
    hl->addWidget(d->stopButton);
    root->addWidget(header);

    // ── Compile diagnostic banner ──
    // A compile error belongs to the run, not to any single case, so it lives
    // here rather than being duplicated into every card.
    d->banner = new QWidget(this);
    auto* bannerLayout = new QVBoxLayout(d->banner);
    bannerLayout->setContentsMargins(Theme::SpaceM, Theme::SpaceS, Theme::SpaceM, Theme::SpaceS);
    bannerLayout->setSpacing(Theme::SpaceXS);

    auto* bannerTitle = new QLabel(tr("COMPILATION FAILED"), d->banner);
    bannerTitle->setFont(Theme::statusFont());
    bannerTitle->setStyleSheet(QString("color: %1; letter-spacing: 1px;").arg(Theme::Failure.name()));

    d->bannerText = new QPlainTextEdit(d->banner);
    d->bannerText->setFont(Theme::terminalFont());
    d->bannerText->setReadOnly(true);
    d->bannerText->setFrameShape(QFrame::NoFrame);
    d->bannerText->setMaximumHeight(150);
    d->bannerText->setStyleSheet(QString(
        "QPlainTextEdit { background: transparent; color: %1; border: none; }"
        "QScrollBar:vertical { width: 4px; background: transparent; }"
        "QScrollBar::handle:vertical { background: %2; border-radius: 2px; min-height: 18px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    ).arg(Theme::TextSecondary.name(QColor::HexArgb),
          Theme::ScrollThumb.name(QColor::HexArgb)));

    bannerLayout->addWidget(bannerTitle);
    bannerLayout->addWidget(d->bannerText);
    d->banner->setStyleSheet(QString("background: %1; border-bottom: 1px solid %2;")
                                 .arg(Theme::FailureBg.name(QColor::HexArgb),
                                      Theme::Border.name(QColor::HexArgb)));
    d->banner->setVisible(false);
    root->addWidget(d->banner);

    // ── Cards ──
    d->cardsHost = new QWidget;
    d->cardsLayout = new QVBoxLayout(d->cardsHost);
    d->cardsLayout->setContentsMargins(Theme::SpaceM, Theme::SpaceM, Theme::SpaceM, Theme::SpaceM);
    d->cardsLayout->setSpacing(Theme::SpaceS);

    d->emptyState = new QLabel(tr("No test cases yet.\nAdd one with +"), d->cardsHost);
    d->emptyState->setFont(Theme::sidebarFont());
    d->emptyState->setAlignment(Qt::AlignCenter);
    d->emptyState->setStyleSheet(QString("color: %1; padding: 24px 0;")
                                     .arg(Theme::TextMuted.name(QColor::HexArgb)));
    d->cardsLayout->addWidget(d->emptyState);
    d->cardsLayout->addStretch();

    d->scroll = new QScrollArea(this);
    d->scroll->setWidget(d->cardsHost);
    d->scroll->setWidgetResizable(true);
    d->scroll->setFrameShape(QFrame::NoFrame);
    d->scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scroll->setStyleSheet(QString(
        "QScrollArea { background: %1; border: none; }"
        "QScrollArea > QWidget > QWidget { background: %1; }"
        "QScrollBar:vertical { width: 6px; background: transparent; margin: 0; }"
        "QScrollBar::handle:vertical { background: %2; border-radius: 3px; min-height: 24px; }"
        "QScrollBar::handle:vertical:hover { background: %3; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
    ).arg(Theme::PanelBg.name(),
          Theme::ScrollThumb.name(QColor::HexArgb),
          Theme::ScrollThumbHover.name(QColor::HexArgb)));
    root->addWidget(d->scroll, 1);

    // ── Engine ──
    d->judge = new Judge(this);

    connect(d->judge, &Judge::compileStarted, this, [this]() {
        d->banner->setVisible(false);
        emit statusMessage(tr("Compiling…"));
    });
    connect(d->judge, &Judge::compileFinished, this, [this](bool ok, const QString& output) {
        if (!ok) {
            d->showBanner(output.trimmed());
            emit statusMessage(tr("Compilation failed"));
        }
    });
    connect(d->judge, &Judge::caseStarted, this, [this](int index) {
        if (index >= 0 && index < d->cards.size()) d->cards[index]->setVerdict(Verdict::Running);
        emit statusMessage(tr("Running test %1…").arg(index + 1));
    });
    connect(d->judge, &Judge::caseFinished, this, [this](int index, const TestCase& result) {
        if (index < 0 || index >= d->cards.size()) return;
        // Keep the user's typed input/expected; only the result fields move.
        TestCase merged = d->cases.at(index);
        merged.actual = result.actual;
        merged.errorText = result.errorText;
        merged.verdict = result.verdict;
        merged.elapsedMs = result.elapsedMs;
        merged.exitCode = result.exitCode;
        d->cases[index] = merged;
        d->cards[index]->applyResult(merged);
        d->updateSummary();
    });
    connect(d->judge, &Judge::allFinished, this, [this](int passed, int total) {
        d->setRunning(false);
        d->updateSummary();
        emit statusMessage(passed == total ? tr("All %1 tests passed").arg(total)
                                           : tr("%1 of %2 tests passed").arg(passed).arg(total));
    });
    connect(d->judge, &Judge::failed, this, [this](const QString& reason) {
        d->setRunning(false);
        d->showBanner(reason);
        emit statusMessage(reason);
    });

    connect(d->addButton,  &QPushButton::clicked, this, &CphPanel::addTestCase);
    connect(d->runButton,  &QPushButton::clicked, this, &CphPanel::runAll);
    connect(d->stopButton, &QPushButton::clicked, this, &CphPanel::stopAll);

    auto* runShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Return")), this);
    runShortcut->setContext(Qt::WindowShortcut);
    connect(runShortcut, &QShortcut::activated, this, &CphPanel::runAll);

    d->updateSummary();
}

CphPanel::~CphPanel() {
    // Kill any live child process before the widgets it reports into go away.
    if (d->judge) d->judge->cancel();
    delete d;
}

// ── Target file ──────────────────────────────────────────────────────────────

QString CphPanel::targetFile() const { return d->target; }

void CphPanel::setTargetFile(const QString& path) {
    if (path == d->target) return;

    persist();                 // keep the outgoing file's cases
    if (d->judge->isBusy()) stopAll();

    d->target = path;
    d->banner->setVisible(false);
    d->clearCards();
    d->cases.clear();
    d->load();

    for (const TestCase& tc : std::as_const(d->cases)) d->addCard(tc, false);
    d->renumber();
    d->updateSummary();
    d->runButton->setEnabled(!d->target.isEmpty());
    d->addButton->setEnabled(!d->target.isEmpty());
}

void CphPanel::persist() {
    if (d->target.isEmpty() || d->cards.isEmpty()) return;
    d->collectFromCards();
    d->save();
}

// ── Slots ────────────────────────────────────────────────────────────────────

void CphPanel::addTestCase() {
    d->collectFromCards();
    d->cases.append(TestCase{});
    d->addCard(d->cases.constLast(), true);
    d->renumber();
    d->updateSummary();
    d->save();

    if (!d->cards.isEmpty()) {
        TestCaseCard* card = d->cards.constLast();
        // Bring the new card into view; it is below the fold once there are a
        // few tests, and silently appending off-screen looks like nothing
        // happened.
        QMetaObject::invokeMethod(this, [this, card]() {
            d->scroll->ensureWidgetVisible(card);
        }, Qt::QueuedConnection);
    }
}

void CphPanel::runAll() {
    if (d->target.isEmpty()) {
        emit statusMessage(tr("Open a C++ file to run tests against."));
        return;
    }
    if (d->judge->isBusy()) return;

    emit saveBeforeRunRequested();     // direct connection: the save completes first

    d->collectFromCards();
    if (d->cases.isEmpty()) {
        emit statusMessage(tr("Add a test case first."));
        return;
    }

    d->save();
    d->banner->setVisible(false);
    for (TestCaseCard* card : std::as_const(d->cards)) card->setVerdict(Verdict::Pending);
    d->setRunning(true);
    d->judge->run(d->target, d->cases);
}

void CphPanel::stopAll() {
    if (!d->judge->isBusy()) return;
    d->judge->cancel();
    d->setRunning(false);
    for (TestCaseCard* card : std::as_const(d->cards)) {
        if (card->data().verdict == Verdict::Running) card->setVerdict(Verdict::Pending);
    }
    emit statusMessage(tr("Run stopped."));
}

// ── Impl ─────────────────────────────────────────────────────────────────────

void CphPanel::Impl::addCard(const TestCase& tc, bool expanded) {
    auto* card = new TestCaseCard(cards.size(), cardsHost);
    card->setData(tc);
    card->setExpanded(expanded);

    // Insert before the trailing stretch.
    cardsLayout->insertWidget(cardsLayout->count() - 1, card);
    cards.append(card);

    QObject::connect(card, &TestCaseCard::edited, q, [this]() {
        collectFromCards();
        updateSummary();
        save();
    });
    QObject::connect(card, &TestCaseCard::removeRequested, q, [this, card]() {
        const int idx = cards.indexOf(card);
        if (idx < 0) return;
        cards.removeAt(idx);
        if (idx < cases.size()) cases.remove(idx);
        card->deleteLater();
        renumber();
        updateSummary();
        save();
    });

    emptyState->setVisible(false);
}

void CphPanel::Impl::clearCards() {
    for (TestCaseCard* card : std::as_const(cards)) {
        cardsLayout->removeWidget(card);
        card->deleteLater();
    }
    cards.clear();
    emptyState->setVisible(true);
}

void CphPanel::Impl::renumber() {
    for (int i = 0; i < cards.size(); i++) cards[i]->setIndex(i);
    emptyState->setVisible(cards.isEmpty());
}

void CphPanel::Impl::collectFromCards() {
    // The cards are the source of truth for what the user typed; the cached
    // results are the source of truth for verdicts.
    QVector<TestCase> next;
    next.reserve(cards.size());
    for (int i = 0; i < cards.size(); i++) {
        TestCase tc = (i < cases.size()) ? cases.at(i) : TestCase{};
        const TestCase typed = cards.at(i)->data();
        tc.input = typed.input;
        tc.expected = typed.expected;
        tc.verdict = typed.verdict;
        next.append(tc);
    }
    cases = next;
}

void CphPanel::Impl::updateSummary() {
    if (cards.isEmpty()) {
        summary->clear();
        summary->setStyleSheet(QString());
        return;
    }

    int passed = 0, judged = 0;
    for (const TestCase& tc : std::as_const(cases)) {
        if (tc.verdict == Verdict::Pending || tc.verdict == Verdict::Running) continue;
        judged++;
        if (tc.verdict == Verdict::Accepted) passed++;
    }

    QColor tint = Theme::Pending;
    QColor fill = Theme::PendingBg;
    if (judged > 0) {
        const bool allPassed = (passed == cases.size());
        tint = allPassed ? Theme::Success : Theme::Failure;
        fill = allPassed ? Theme::SuccessBg : Theme::FailureBg;
    }

    summary->setText(QStringLiteral("%1/%2").arg(passed).arg(cases.size()));
    summary->setStyleSheet(QString("background: %1; color: %2; border-radius: %3px; padding: 2px 8px;")
                               .arg(fill.name(QColor::HexArgb), tint.name(),
                                    QString::number(Theme::Radius - 2)));
}

void CphPanel::Impl::setRunning(bool running) {
    // Run and Stop are the same slot in the layout — showing both at once would
    // leave the user guessing which one is live.
    runButton->setVisible(!running);
    stopButton->setVisible(running);
    addButton->setEnabled(!running && !target.isEmpty());
}

void CphPanel::Impl::showBanner(const QString& text) {
    bannerText->setPlainText(text);
    banner->setVisible(!text.isEmpty());
}

// ── Persistence ──────────────────────────────────────────────────────────────

QString CphPanel::Impl::sidecarPath() const {
    if (target.isEmpty()) return QString();
    const QFileInfo fi(target);
    return fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName() +
           QStringLiteral(".valence-tests.json");
}

void CphPanel::Impl::load() {
    const QString path = sidecarPath();
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QByteArray raw = f.readAll();
    f.close();

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;

    const QJsonArray arr = doc.object().value(QStringLiteral("cases")).toArray();
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        TestCase tc;
        tc.input = o.value(QStringLiteral("input")).toString();
        tc.expected = o.value(QStringLiteral("expected")).toString();
        cases.append(tc);
    }
}

void CphPanel::Impl::save() const {
    const QString path = sidecarPath();
    if (path.isEmpty()) return;

    // Nothing to remember: remove a stale sidecar rather than leaving an empty
    // one littering the problem folder.
    if (cases.isEmpty()) {
        QFile::remove(path);
        return;
    }

    QJsonArray arr;
    for (const TestCase& tc : cases) {
        QJsonObject o;
        o.insert(QStringLiteral("input"), tc.input);
        o.insert(QStringLiteral("expected"), tc.expected);
        arr.append(o);
    }
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("cases"), arr);

    // QSaveFile writes to a temporary and renames, so a crash mid-write cannot
    // truncate the user's saved tests.
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.commit();
}

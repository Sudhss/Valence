#include "tab_widget.h"
#include "../theme/theme.h"
#include <QVBoxLayout>
#include <QMouseEvent>

TabWidget::TabWidget(QWidget* parent) : QWidget(parent) {
    stack_ = new QStackedWidget(this);

    // Empty state joke label
    jokeLabel_ = new QLabel("No folder opened.\nAre you just here to admire the dark theme?", this);
    jokeLabel_->setAlignment(Qt::AlignCenter);
    jokeLabel_->setFont(Theme::statusFont());
    jokeLabel_->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 500;").arg(Theme::TextMuted.name(QColor::HexArgb)));

    tabs_ = new QTabWidget(this);
    tabs_->setTabsClosable(true);
    tabs_->setMovable(true);
    tabs_->setDocumentMode(true);

    stack_->addWidget(jokeLabel_); // Index 0: Empty state
    stack_->addWidget(tabs_);      // Index 1: Tabs

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(stack_);

    connect(tabs_, &QTabWidget::currentChanged, this, &TabWidget::currentChanged);
    connect(tabs_, &QTabWidget::tabCloseRequested, this, &TabWidget::tabCloseRequested);

    // Middle-click to close, which is muscle memory from every browser and
    // every other editor.
    tabs_->tabBar()->installEventFilter(this);
    tabs_->tabBar()->setElideMode(Qt::ElideMiddle);   // keep the extension visible

    applyStyle();
    updateStackVisibility();
}

bool TabWidget::eventFilter(QObject* obj, QEvent* e) {
    if (obj == tabs_->tabBar() && e->type() == QEvent::MouseButtonRelease) {
        auto* me = static_cast<QMouseEvent*>(e);
        if (me->button() == Qt::MiddleButton) {
            const int idx = tabs_->tabBar()->tabAt(me->position().toPoint());
            if (idx >= 0) {
                emit tabCloseRequested(idx);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, e);
}

void TabWidget::updateStackVisibility() {
    stack_->setCurrentIndex(tabs_->count() > 0 ? 1 : 0);
}

void TabWidget::applyStyle() {
    tabs_->setStyleSheet(QString(
        "QTabWidget::pane {"
        "  border: none;"
        "  background: %1;"
        "}"
        "QTabBar {"
        "  background: %2;"
        "  qproperty-drawBase: 0;"
        "}"
        // Inactive tabs sit back on the chrome surface with no fill of their
        // own, so the active one is the only thing that reads as forward.
        "QTabBar::tab {"
        "  background: transparent;"
        "  color: %4;"
        "  padding: 10px 16px;"
        "  border: none;"
        "  border-top: 2px solid transparent;"
        "  border-right: 1px solid %3;"
        "  font-size: 13px;"
        "  min-width: 0px;"
        "  margin: 0;"
        "}"
        // The active tab is a raised surface lifted out of the strip, lit along
        // its top edge by the accent, and sharing the editor's colour so the
        // two read as one continuous plane. An underline alone leaves every tab
        // looking equally flat.
        "QTabBar::tab:selected {"
        "  color: %5;"
        "  background: %1;"
        "  border-top: 2px solid %6;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  color: %5;"
        "  background: %7;"
        "}"
        "QTabBar::close-button {"
        "  image: none;"
        "  width: 14px;"
        "  height: 14px;"
        "  background: transparent;"
        "  border-radius: 7px;"
        "  margin: 8px 2px 8px 6px;"
        "}"
        "QTabBar::close-button:hover {"
        "  background: rgba(255, 255, 255, 0.12);"
        "}"
    ).arg(Theme::Surface.name(),                        // %1 active tab + pane
          Theme::Chrome.name(),                         // %2 the strip behind them
          Theme::Border.name(QColor::HexArgb),          // %3 separator
          Theme::TextMuted.name(QColor::HexArgb),       // %4 inactive label
          Theme::TextPrimary.name(),                    // %5 active label
          Theme::Accent.name(),                         // %6 top indicator
          Theme::HoverWash.name(QColor::HexArgb)));     // %7 hover
}

int TabWidget::addEditor(EditorWidget* editor, const QString& label) {
    int idx = tabs_->addTab(editor, label);
    tabs_->setTabToolTip(idx, editor->filePath().isEmpty() ? label : editor->filePath());
    tabs_->setCurrentIndex(idx);
    updateStackVisibility();
    return idx;
}

EditorWidget* TabWidget::currentEditor() const {
    return qobject_cast<EditorWidget*>(tabs_->currentWidget());
}

EditorWidget* TabWidget::editorAt(int index) const {
    return qobject_cast<EditorWidget*>(tabs_->widget(index));
}

int TabWidget::findByFilePath(const QString& path) const {
    for (int i = 0; i < tabs_->count(); i++) {
        auto* editor = editorAt(i);
        if (editor && editor->filePath() == path) return i;
    }
    return -1;
}

void TabWidget::setCurrentIndex(int index) { tabs_->setCurrentIndex(index); }
int TabWidget::currentIndex() const { return tabs_->currentIndex(); }
int TabWidget::count() const { return tabs_->count(); }

void TabWidget::updateTabLabel(int index) {
    auto* editor = editorAt(index);
    if (!editor) return;
    QString label = editor->fileName();
    if (editor->isModified()) label += " ●";
    tabs_->setTabText(index, label);
    // The name alone is ambiguous once two problems are both called main.cpp.
    tabs_->setTabToolTip(index, editor->filePath().isEmpty() ? label : editor->filePath());
}

void TabWidget::closeTab(int index) {
    QWidget* w = tabs_->widget(index);
    tabs_->removeTab(index);
    delete w;
    updateStackVisibility();
}

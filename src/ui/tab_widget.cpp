#include "tab_widget.h"
#include "../theme/theme.h"
#include <QVBoxLayout>

TabWidget::TabWidget(QWidget* parent) : QWidget(parent) {
    tabs_ = new QTabWidget(this);
    tabs_->setTabsClosable(true);
    tabs_->setMovable(true);
    tabs_->setDocumentMode(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(tabs_);

    connect(tabs_, &QTabWidget::currentChanged, this, &TabWidget::currentChanged);
    connect(tabs_, &QTabWidget::tabCloseRequested, this, &TabWidget::tabCloseRequested);

    applyStyle();
}

void TabWidget::applyStyle() {
    tabs_->setStyleSheet(QString(
        "QTabWidget::pane {"
        "  border: none;"
        "  background: %1;"
        "}"
        "QTabBar {"
        "  background: %2;"
        "  border-bottom: 1px solid %3;"
        "  qproperty-drawBase: 0;"
        "}"
        "QTabBar::tab {"
        "  background: transparent;"
        "  color: %4;"
        "  padding: 10px 24px;"
        "  border: none;"
        "  border-bottom: 2px solid transparent;"
        "  font-family: 'JetBrains Mono', 'Consolas', monospace;"
        "  font-size: 11px;"
        "  min-width: 100px;"
        "  margin: 0;"
        "}"
        "QTabBar::tab:selected {"
        "  color: %5;"
        "  border-bottom: 2px solid %6;"
        "  background: rgba(255, 255, 255, 0.02);"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  color: %5;"
        "  background: rgba(255, 255, 255, 0.02);"
        "}"
        "QTabBar::close-button {"
        "  image: none;"
        "  width: 6px;"
        "  height: 6px;"
        "  background: rgba(255, 255, 255, 0.15);"
        "  border-radius: 3px;"
        "  margin: 6px 4px 6px 0px;"
        "}"
        "QTabBar::close-button:hover {"
        "  background: rgba(255, 100, 80, 0.7);"
        "}"
    ).arg(Theme::EditorBg.name(),
          Theme::TitlebarBg.name(),
          Theme::Border.name(QColor::HexArgb),
          Theme::TextMuted.name(QColor::HexArgb),
          Theme::TextPrimary.name(),
          Theme::Accent.name()));
}

int TabWidget::addEditor(EditorWidget* editor, const QString& label) {
    int idx = tabs_->addTab(editor, label);
    tabs_->setCurrentIndex(idx);
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
}

void TabWidget::closeTab(int index) {
    QWidget* w = tabs_->widget(index);
    tabs_->removeTab(index);
    delete w;
}

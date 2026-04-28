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
        "}"
        "QTabBar::tab {"
        "  background: transparent;"
        "  color: %4;"
        "  padding: 8px 20px;"
        "  border: none;"
        "  border-bottom: 2px solid transparent;"
        "  font-family: 'JetBrains Mono', 'Consolas', monospace;"
        "  font-size: 12px;"
        "  min-width: 80px;"
        "}"
        "QTabBar::tab:selected {"
        "  color: %5;"
        "  border-bottom: 2px solid %6;"
        "}"
        "QTabBar::tab:hover {"
        "  color: %5;"
        "  background: rgba(255, 255, 255, 0.03);"
        "}"
        "QTabBar::close-button {"
        "  image: none;"
        "  width: 8px;"
        "  height: 8px;"
        "  background: rgba(255, 255, 255, 0.2);"
        "  border-radius: 4px;"
        "  margin: 4px;"
        "}"
        "QTabBar::close-button:hover {"
        "  background: rgba(255, 69, 58, 0.8);"
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

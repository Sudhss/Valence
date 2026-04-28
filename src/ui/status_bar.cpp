#include "status_bar.h"
#include "../theme/theme.h"
#include <QHBoxLayout>

StatusBar::StatusBar(QWidget* parent) : QWidget(parent) {
    setFixedHeight(24);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(0);

    auto makeLabel = [this](const QString& text, Qt::Alignment align = Qt::AlignLeft) {
        auto* label = new QLabel(text, this);
        label->setFont(Theme::statusFont());
        label->setAlignment(align | Qt::AlignVCenter);
        label->setStyleSheet(QString("color: %1; padding: 0 8px;")
            .arg(Theme::TextMuted.name(QColor::HexArgb)));
        return label;
    };

    fileLabel_ = makeLabel("");
    langLabel_ = makeLabel("C++");
    encLabel_ = makeLabel("UTF-8");
    posLabel_ = makeLabel("Ln 1, Col 1", Qt::AlignRight);

    layout->addWidget(fileLabel_);
    layout->addWidget(langLabel_);
    layout->addStretch();
    layout->addWidget(encLabel_);
    layout->addWidget(posLabel_);

    setStyleSheet(QString(
        "background: %1; border-top: 1px solid %2;"
    ).arg(Theme::SidebarBg.name(), Theme::Border.name(QColor::HexArgb)));
}

void StatusBar::setFileName(const QString& name) {
    fileLabel_->setText(name);
}

void StatusBar::setCursorPosition(int row, int col) {
    posLabel_->setText(QString("Ln %1, Col %2").arg(row + 1).arg(col + 1));
}

void StatusBar::setLanguage(const QString& lang) {
    langLabel_->setText(lang);
}

void StatusBar::setEncoding(const QString& enc) {
    encLabel_->setText(enc);
}

#include "snippet_popup.h"
#include "../theme/theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>

namespace {
constexpr int kPadV = 5;    // vertical padding inside the panel
constexpr int kPadH = 10;   // horizontal padding inside a row
constexpr int kGap  = 18;   // gap between the trigger and its description
constexpr int kShadow = 8;  // margin reserved around the panel for its shadow
}

SnippetPopup::SnippetPopup(QWidget* parent) : QWidget(parent) {
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents);   // never steals a click
    hide();
}

int SnippetPopup::rowHeight() const {
    return QFontMetrics(Theme::uiFont()).height() + 8;
}

QSize SnippetPopup::sizeForItems() const {
    const QFontMetrics fmName(Theme::codeFont(Theme::FontSizeUI));
    const QFontMetrics fmDetail(Theme::uiFont(Theme::FontSizeSmall));

    int widest = 0;
    for (const Snippet& s : items_) {
        widest = qMax(widest, fmName.horizontalAdvance(s.trigger) + kGap +
                                  fmDetail.horizontalAdvance(s.detail));
    }
    return QSize(widest + kPadH * 2 + kShadow * 2,
                 items_.size() * rowHeight() + kPadV * 2 + kShadow * 2);
}

void SnippetPopup::showFor(const QVector<Snippet>& items, const QPoint& anchor) {
    if (items.isEmpty()) { hide(); items_.clear(); return; }

    // Reset the highlight whenever the offered set changes, so the selection
    // never points at a row that is no longer there.
    if (items.size() != items_.size() ||
        (!items.isEmpty() && !items_.isEmpty() && items.first().trigger != items_.first().trigger)) {
        selected_ = 0;
    }
    items_ = items;
    selected_ = qBound(0, selected_, items_.size() - 1);

    const QSize wanted = sizeForItems();
    QPoint pos = anchor;

    // Keep the panel inside the editor: flip above the caret when there is no
    // room below, and pull back from the right edge rather than being clipped.
    if (parentWidget()) {
        const int maxX = parentWidget()->width() - wanted.width() - 4;
        pos.setX(qBound(4, pos.x(), qMax(4, maxX)));
        if (pos.y() + wanted.height() > parentWidget()->height()) {
            // 4px above the caret's top; anchor.y() is its bottom.
            pos.setY(qMax(0, anchor.y() - wanted.height() - QFontMetrics(Theme::codeFont()).height() - 4));
        }
    }

    setGeometry(QRect(pos, wanted));
    raise();
    show();
    update();
}

void SnippetPopup::moveSelection(int delta) {
    if (items_.isEmpty()) return;
    // Wraps, so Up from the first row lands on the last.
    selected_ = (selected_ + delta + items_.size()) % items_.size();
    update();
}

const Snippet* SnippetPopup::selected() const {
    if (items_.isEmpty()) return nullptr;
    return &items_.at(qBound(0, selected_, items_.size() - 1));
}

void SnippetPopup::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const QRectF panel = QRectF(rect()).adjusted(kShadow + 0.5, 0.5,
                                                 -kShadow - 0.5, -kShadow - 0.5);
    QPainterPath path;
    path.addRoundedRect(panel, Theme::RadiusPanel, Theme::RadiusPanel);

    // A soft drop shadow, built from concentric rounded rects at falling alpha.
    // Something floating over code with no shadow is the clearest sign of a
    // flat interface, and this costs a handful of fills rather than a
    // QGraphicsEffect and its offscreen buffer.
    for (int i = kShadow; i > 0; i--) {
        QColor tint = Theme::ShadowSoft;
        tint.setAlpha(Theme::ShadowSoft.alpha() * (kShadow - i + 1) / (kShadow * 4));
        QPainterPath glow;
        glow.addRoundedRect(panel.adjusted(-i, -i * 0.4, i, i * 1.2),
                            Theme::RadiusPanel + i, Theme::RadiusPanel + i);
        p.fillPath(glow, tint);
    }

    // Opaque: this floats over code and has to stay readable.
    p.fillPath(path, Theme::Overlay);
    p.setPen(QPen(Theme::BorderMedium, 1));
    p.drawPath(path);

    // Lit along the top edge, like every other raised surface in the app.
    p.setPen(QPen(Theme::HighlightTop, 1));
    p.drawLine(QPointF(panel.left() + Theme::RadiusPanel, panel.top() + 0.5),
               QPointF(panel.right() - Theme::RadiusPanel, panel.top() + 0.5));

    const QFont nameFont = Theme::codeFont(Theme::FontSizeUI);
    const QFont detailFont = Theme::uiFont(Theme::FontSizeSmall);
    const int rh = rowHeight();

    for (int i = 0; i < items_.size(); i++) {
        const QRect row(kShadow + 1, kShadow + kPadV + i * rh,
                        width() - kShadow * 2 - 2, rh);

        if (i == selected_) {
            QPainterPath sel;
            sel.addRoundedRect(QRectF(row).adjusted(3, 0, -3, 0),
                               Theme::Radius - 2, Theme::Radius - 2);
            p.fillPath(sel, Theme::AccentDim);
        }

        p.setFont(nameFont);
        p.setPen(i == selected_ ? Theme::Accent : Theme::TextPrimary);
        p.drawText(row.adjusted(kPadH, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter,
                   items_.at(i).trigger);

        p.setFont(detailFont);
        p.setPen(Theme::TextMuted);
        p.drawText(row.adjusted(0, 0, -kPadH, 0), Qt::AlignRight | Qt::AlignVCenter,
                   items_.at(i).detail);
    }
}

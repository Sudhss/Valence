#pragma once
#include <QWidget>
#include <QVector>
#include "snippets.h"

// The suggestion list shown under the caret while a snippet trigger is being
// typed.
//
// A plain child widget of the editor rather than a Qt::Popup window: a popup
// would grab the keyboard, and the editor needs to keep handling keys so that
// typing carries on normally while the list is open. It never takes focus.
class SnippetPopup : public QWidget {
    Q_OBJECT

public:
    explicit SnippetPopup(QWidget* parent = nullptr);

    // Shows the list at `anchor` (a point in parent coordinates, the caret's
    // bottom-left). Hides itself if there is nothing to offer.
    void showFor(const QVector<Snippet>& items, const QPoint& anchor);

    bool hasItems() const { return !items_.isEmpty(); }
    void moveSelection(int delta);
    const Snippet* selected() const;

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    QVector<Snippet> items_;
    int selected_ = 0;

    int rowHeight() const;
    QSize sizeForItems() const;
};

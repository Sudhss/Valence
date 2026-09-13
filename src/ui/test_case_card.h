#pragma once
#include <QWidget>
#include "../cph/test_case.h"

class QLabel;
class QPlainTextEdit;
class QPushButton;

// One test case in the judge panel.
//
// Collapsed it is a single row — index, verdict, elapsed time — which is what
// keeps a ten-case panel readable. Expanded it shows the input and expected
// output, plus the actual output once a run has disagreed with it.
class TestCaseCard : public QWidget {
    Q_OBJECT

public:
    explicit TestCaseCard(int index, QWidget* parent = nullptr);

    void setIndex(int index);
    int  index() const { return index_; }

    // Input and expected as currently typed. Verdict/actual are not read back
    // from the widgets — the panel owns those.
    TestCase data() const;
    void setData(const TestCase& tc);
    void applyResult(const TestCase& result);
    void setVerdict(Verdict v);

    void setExpanded(bool on);
    bool isExpanded() const { return expanded_; }

signals:
    void edited();              // the user changed input or expected
    void removeRequested();

protected:
    void paintEvent(QPaintEvent* e) override;
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    void refreshBadge();
    void autoSize(QPlainTextEdit* edit);

    int index_;
    bool expanded_ = true;
    Verdict verdict_ = Verdict::Pending;

    QWidget* header_ = nullptr;
    QLabel*  indexLabel_ = nullptr;
    QLabel*  badge_ = nullptr;
    QLabel*  timeLabel_ = nullptr;
    QPushButton* removeButton_ = nullptr;

    QWidget* body_ = nullptr;
    QPlainTextEdit* input_ = nullptr;
    QPlainTextEdit* expected_ = nullptr;

    QWidget* actualBlock_ = nullptr;
    QLabel*  actualLabel_ = nullptr;
    QPlainTextEdit* actual_ = nullptr;
};

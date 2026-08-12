#pragma once

#include <QLayout>
#include <QStyle>

// ---------------------------------------------------------------------------
// FlowLayout - the canonical Qt flow layout: children wrap into rows and
// grow vertically. Used for country/category/language chips.
// ---------------------------------------------------------------------------
class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget* parent, int margin = 0, int hSpacing = 8, int vSpacing = 8);
    explicit FlowLayout(int margin = 0, int hSpacing = 8, int vSpacing = 8);
    ~FlowLayout() override;

    void addItem(QLayoutItem* item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    int count() const override;
    QLayoutItem* itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect& rect) override;
    QSize sizeHint() const override;
    QLayoutItem* takeAt(int index) override;

private:
    void doLayout(const QRect& rect, bool testOnly) const;
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem*> m_items;
    int m_hSpace;
    int m_vSpace;
};

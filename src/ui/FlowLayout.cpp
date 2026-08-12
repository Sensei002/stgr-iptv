#include "ui/FlowLayout.h"

#include <QWidget>

FlowLayout::FlowLayout(QWidget* parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent)
    , m_hSpace(hSpacing)
    , m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::FlowLayout(int margin, int hSpacing, int vSpacing)
    : m_hSpace(hSpacing)
    , m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    while (!m_items.isEmpty())
        delete m_items.takeFirst();
}

void FlowLayout::addItem(QLayoutItem* item)
{
    m_items.append(item);
}

int FlowLayout::horizontalSpacing() const
{
    return m_hSpace >= 0 ? m_hSpace : smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int FlowLayout::verticalSpacing() const
{
    return m_vSpace >= 0 ? m_vSpace : smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

int FlowLayout::count() const
{
    return m_items.size();
}

QLayoutItem* FlowLayout::itemAt(int index) const
{
    return m_items.value(index);
}

QLayoutItem* FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < m_items.size())
        return m_items.takeAt(index);
    return nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    const int effectiveWidth = width - contentsMargins().left() - contentsMargins().right();
    int height = 0;
    int x = 0;
    int rowHeight = 0;

    for (QLayoutItem* item : m_items) {
        const int w = item->sizeHint().width();
        const int h = item->sizeHint().height();
        if (x + w > effectiveWidth && rowHeight > 0) {
            height += rowHeight + verticalSpacing();
            x = 0;
            rowHeight = 0;
        }
        x += w + horizontalSpacing();
        rowHeight = qMax(rowHeight, h);
    }
    if (rowHeight > 0)
        height += rowHeight;

    return height + contentsMargins().top() + contentsMargins().bottom();
}

void FlowLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (QLayoutItem* item : m_items)
        size = size.expandedTo(item->minimumSize());
    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
}

void FlowLayout::doLayout(const QRect& rect, bool testOnly) const
{
    int x = rect.x();
    int y = rect.y();
    int rowHeight = 0;

    for (QLayoutItem* item : m_items) {
        const int w = item->sizeHint().width();
        const int h = item->sizeHint().height();

        if (x + w > rect.right() + 1 && rowHeight > 0) {
            x = rect.x();
            y = y + rowHeight + verticalSpacing();
            rowHeight = 0;
        }

        if (!testOnly)
            item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

        x += w + horizontalSpacing();
        rowHeight = qMax(rowHeight, h);
    }
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject* parentObject = parent();
    if (!parentObject)
        return -1;
    if (parentObject->isWidgetType()) {
        auto* pw = static_cast<QWidget*>(parentObject);
        return pw->style()->pixelMetric(pm, nullptr, pw);
    }
    return static_cast<QLayout*>(parentObject)->spacing();
}

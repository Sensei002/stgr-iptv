#pragma once

#include <QStyledItemDelegate>

// ---------------------------------------------------------------------------
// ChannelDelegate - paints channel cards in two modes:
//   Mode::List - dense rows with logo, name, metadata and status.
//   Mode::Grid - tiles sized to the view's gridSize.
//
// Logos are pulled lazily through LogoCache (request + cached pixmap) so
// thousands of channels never cause thousands of downloads.
// ---------------------------------------------------------------------------
class ChannelDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    enum class Mode { List, Grid };

    explicit ChannelDelegate(QObject* parent = nullptr);

    void setMode(Mode mode) { m_mode = mode; }
    Mode mode() const { return m_mode; }
    void setShowLogos(bool show) { m_showLogos = show; }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

private:
    void paintList(QPainter* painter, const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;
    void paintGrid(QPainter* painter, const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;

    Mode m_mode = Mode::Grid;
    bool m_showLogos = true;
};

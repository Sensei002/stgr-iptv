#include "ui/ChannelDelegate.h"

#include <QPainter>
#include <QPainterPath>

#include "services/LogoCache.h"
#include "ui/ChannelListModel.h"
#include "ui/Theme.h"

namespace {

QString firstLetter(const QString& name)
{
    return name.isEmpty() ? QStringLiteral("?") : name.left(1).toUpper();
}

} // namespace

ChannelDelegate::ChannelDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QSize ChannelDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (m_mode == Mode::Grid)
        return option.rect.size().isValid() ? option.rect.size() : QSize(190, 128);
    return QSize(option.rect.width(), 56);
}

void ChannelDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                            const QModelIndex& index) const
{
    if (m_mode == Mode::Grid)
        paintGrid(painter, option, index);
    else
        paintList(painter, option, index);
}

void ChannelDelegate::paintList(QPainter* painter, const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const QRect r = option.rect;
    const Theme::Colors& c = Theme::colors();
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;

    // Background
    QColor bg = selected ? c.accentSoft : Qt::transparent;
    if (hovered && !selected)
        bg = QColor(0x1f, 0x1f, 0x27);
    if (bg.alpha() > 0)
        painter->fillRect(r.adjusted(2, 1, -2, -1), bg);

    if (selected)
        painter->fillRect(QRect(r.left() + 2, r.top() + 8, 3, r.height() - 16), c.accent);

    const QString name = index.data(ChannelListModel::NameRole).toString();
    const QString subtitle = index.data(ChannelListModel::SubtitleRole).toString();
    const bool favorite = index.data(ChannelListModel::FavoriteRole).toBool();
    const bool playing = index.data(ChannelListModel::IsPlayingRole).toBool();
    const QString logoUrl = index.data(ChannelListModel::LogoUrlRole).toString();

    // Logo
    const QRect logoRect(r.left() + 12, r.top() + 10, 36, 36);
    QPixmap logo = LogoCache::instance()->cachedPixmap(logoUrl);
    if (!logoUrl.isEmpty() && !logo.isNull()) {
        QPainterPath clip;
        clip.addRoundedRect(QRectF(logoRect), 7, 7);
        painter->save();
        painter->setClipPath(clip);
        painter->drawPixmap(logoRect, logo.scaled(logoRect.size(),
                                                  Qt::KeepAspectRatioByExpanding,
                                                  Qt::SmoothTransformation));
        painter->restore();
    } else {
        LogoCache::instance()->request(logoUrl, logoUrl);
        // Fallback monogram
        painter->setPen(Qt::NoPen);
        painter->setBrush(c.accentSoft);
        painter->drawRoundedRect(QRectF(logoRect), 7, 7);
        painter->setPen(c.accent);
        QFont f = painter->font();
        f.setBold(true);
        f.setPointSizeF(11);
        painter->setFont(f);
        painter->drawText(logoRect, Qt::AlignCenter, firstLetter(name));
    }

    // Text
    const int textLeft = logoRect.right() + 12;
    const int textRight = r.right() - 44;
    QFont bold = painter->font();
    bold.setWeight(QFont::DemiBold);
    painter->setFont(bold);
    painter->setPen(selected ? c.accentHover : c.text);
    painter->drawText(QRect(textLeft, r.top() + 8, textRight - textLeft, 20),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(name, Qt::ElideRight, textRight - textLeft));

    QFont normal = painter->font();
    normal.setWeight(QFont::Normal);
    painter->setFont(normal);
    painter->setPen(c.textDim);
    painter->drawText(QRect(textLeft, r.top() + 30, textRight - textLeft, 16),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(subtitle, Qt::ElideRight, textRight - textLeft));

    // Right side: favorite star + playing state
    if (playing) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(c.accent);
        painter->drawEllipse(QPointF(r.right() - 18, r.center().y() - 6), 4, 4);
        painter->setPen(c.accent);
        QFont tiny = painter->font();
        tiny.setPointSizeF(7.5);
        tiny.setWeight(QFont::Bold);
        painter->setFont(tiny);
        painter->drawText(QRect(r.right() - 62, r.center().y() - 12, 42, 24),
                          Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("LIVE"));
    } else if (favorite) {
        const QIcon star = Theme::icon(QStringLiteral("star"), c.gold, 18);
        star.paint(painter, QRect(r.right() - 26, r.center().y() - 9, 18, 18));
    }

    painter->restore();
}

void ChannelDelegate::paintGrid(QPainter* painter, const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const QRect r = option.rect;
    const Theme::Colors& c = Theme::colors();
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;

    // Card background
    QPainterPath card;
    card.addRoundedRect(QRectF(r.adjusted(2, 2, -2, -2)), 10, 10);
    painter->fillPath(card, selected ? c.accentSoft : QColor(0x18, 0x18, 0x1f));

    QColor border = selected ? c.accent : (hovered ? QColor(0x3a, 0x3a, 0x46) : c.border);
    painter->setPen(QPen(border, selected ? 1.6 : 1.0));
    painter->drawPath(card);

    const QString name = index.data(ChannelListModel::NameRole).toString();
    const QString subtitle = index.data(ChannelListModel::SubtitleRole).toString();
    const bool favorite = index.data(ChannelListModel::FavoriteRole).toBool();
    const bool playing = index.data(ChannelListModel::IsPlayingRole).toBool();
    const QString logoUrl = index.data(ChannelListModel::LogoUrlRole).toString();

    // Logo
    const QRect logoRect(r.left() + (r.width() - 52) / 2, r.top() + 16, 52, 52);
    QPixmap logo = LogoCache::instance()->cachedPixmap(logoUrl);
    if (!logoUrl.isEmpty() && !logo.isNull()) {
        QPainterPath clip;
        clip.addRoundedRect(QRectF(logoRect), 10, 10);
        painter->save();
        painter->setClipPath(clip);
        painter->drawPixmap(logoRect, logo.scaled(logoRect.size(),
                                                  Qt::KeepAspectRatioByExpanding,
                                                  Qt::SmoothTransformation));
        painter->restore();
    } else {
        LogoCache::instance()->request(logoUrl, logoUrl);
        painter->setPen(Qt::NoPen);
        painter->setBrush(selected ? QColor(0x4a, 0x1a, 0x20) : c.panelAlt);
        painter->drawRoundedRect(QRectF(logoRect), 10, 10);
        painter->setPen(c.accent);
        QFont f = painter->font();
        f.setBold(true);
        f.setPointSizeF(14);
        painter->setFont(f);
        painter->drawText(logoRect, Qt::AlignCenter, firstLetter(name));
    }

    // Favorite / playing badge (top-right corner)
    if (playing) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(c.accent);
        painter->drawEllipse(QPointF(r.right() - 14, r.top() + 14), 5, 5);
    } else if (favorite) {
        const QIcon star = Theme::icon(QStringLiteral("star"), c.gold, 14);
        star.paint(painter, QRect(r.right() - 20, r.top() + 8, 14, 14));
    }

    // Name + subtitle
    const int textW = r.width() - 20;
    QFont nameFont = painter->font();
    nameFont.setWeight(QFont::DemiBold);
    painter->setFont(nameFont);
    painter->setPen(c.text);
    const QString elidedName = painter->fontMetrics().elidedText(name, Qt::ElideRight, textW);
    painter->drawText(QRect(r.left() + 10, r.bottom() - 44, textW, 18),
                      Qt::AlignHCenter | Qt::AlignVCenter, elidedName);

    QFont subFont = painter->font();
    subFont.setPointSizeF(8.5);
    painter->setFont(subFont);
    painter->setPen(c.textDim);
    const QString elidedSub = painter->fontMetrics().elidedText(subtitle, Qt::ElideRight, textW);
    painter->drawText(QRect(r.left() + 10, r.bottom() - 24, textW, 16),
                      Qt::AlignHCenter | Qt::AlignVCenter, elidedSub);

    painter->restore();
}

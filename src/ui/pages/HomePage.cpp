#include "ui/pages/HomePage.h"

#include <QHash>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

#include "services/FavoritesStore.h"
#include "services/HistoryStore.h"
#include "ui/ChannelView.h"
#include "ui/ChipButton.h"
#include "ui/FlowLayout.h"
#include "ui/Theme.h"
#include "utils/StringUtils.h"

namespace {
constexpr int kMaxShortcutChips = 14;
}

HomePage::HomePage(QWidget* parent)
    : QWidget(parent)
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setObjectName(QStringLiteral("homeScroll"));
    // Let the dojo decoration painted on this page show through.
    scroll->viewport()->setAutoFillBackground(false);

    auto* content = new QWidget(scroll);
    m_rootLayout = new QVBoxLayout(content);
    m_rootLayout->setContentsMargins(24, 20, 24, 24);
    m_rootLayout->setSpacing(6);

    auto* title = new QLabel(tr("Home"), content);
    title->setProperty("stgrClass", QStringLiteral("pageTitle"));
    m_rootLayout->addWidget(title);

    auto* subtitle = new QLabel(tr("Welcome to STGR IpTV \u2014 pick a channel and start watching."), content);
    subtitle->setProperty("stgrClass", QStringLiteral("dim"));
    m_rootLayout->addWidget(subtitle);
    m_rootLayout->addSpacing(14);

    // Continue watching
    m_continueTitle = buildSectionTitle(tr("CONTINUE WATCHING"));
    m_rootLayout->addWidget(m_continueTitle);
    m_continueView = new ChannelView(content);
    m_continueView->setHorizontalStrip(true);
    m_continueView->setFixedHeight(128);
    m_continueView->setEmptyState(tr("Nothing watched yet"),
                                  tr("Channels you watch will show up here."));
    connect(m_continueView, &ChannelView::channelActivated,
            this, [this](const Channel& ch) {
                emit channelActivated(ch, m_continueView->channels());
            });
    m_rootLayout->addWidget(m_continueView);
    m_rootLayout->addSpacing(14);

    // Favorites
    m_favoritesTitle = buildSectionTitle(tr("FAVORITES"));
    m_rootLayout->addWidget(m_favoritesTitle);
    m_favoritesView = new ChannelView(content);
    m_favoritesView->setHorizontalStrip(true);
    m_favoritesView->setFixedHeight(128);
    m_favoritesView->setEmptyState(tr("No favorites yet"),
                                  tr("Tap the star on any channel to pin it here."));
    connect(m_favoritesView, &ChannelView::channelActivated,
            this, [this](const Channel& ch) {
                emit channelActivated(ch, m_favoritesView->channels());
            });
    m_rootLayout->addWidget(m_favoritesView);
    m_rootLayout->addSpacing(14);

    // Country shortcuts
    m_countriesTitle = buildSectionTitle(tr("COUNTRIES"));
    m_rootLayout->addWidget(m_countriesTitle);
    auto* countryHost = new QWidget(content);
    m_countryChips = new FlowLayout(countryHost, 0, 8, 8);
    m_rootLayout->addWidget(countryHost);
    m_rootLayout->addSpacing(14);

    // Category shortcuts
    m_categoriesTitle = buildSectionTitle(tr("CATEGORIES"));
    m_rootLayout->addWidget(m_categoriesTitle);
    auto* categoryHost = new QWidget(content);
    m_categoryChips = new FlowLayout(categoryHost, 0, 8, 8);
    m_rootLayout->addWidget(categoryHost);
    m_rootLayout->addStretch(1);

    scroll->setWidget(content);
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);

    // Default states before any data arrives.
    m_continueTitle->setVisible(false);
    m_favoritesTitle->setVisible(false);
    m_countriesTitle->setVisible(false);
    m_categoriesTitle->setVisible(false);

    rebuildChips();
}

QWidget* HomePage::buildSectionTitle(const QString& text)
{
    auto* label = new QLabel(text, this);
    label->setProperty("stgrClass", QStringLiteral("sectionTitle"));
    return label;
}

void HomePage::setAllChannels(const QVector<Channel>& channels)
{
    m_all = channels;
    rebuildChips();
}

void HomePage::setFavorites(const QVector<Channel>& favorites)
{
    const bool any = !favorites.isEmpty();
    m_favoritesTitle->setVisible(any);
    m_favoritesView->setVisible(any);
    m_favoritesView->setChannels(favorites);
}

void HomePage::setHistory(const QVector<Channel>& history)
{
    const bool any = !history.isEmpty();
    m_continueTitle->setVisible(any);
    m_continueView->setVisible(any);
    m_continueView->setChannels(history);
}

void HomePage::setCurrentKey(const QString& key)
{
    m_continueView->setCurrentKey(key);
    m_favoritesView->setCurrentKey(key);
}

void HomePage::refreshShortcuts()
{
    rebuildChips();
}

void HomePage::rebuildChips()
{
    // Clear existing chips.
    while (QLayoutItem* item = m_countryChips->takeAt(0)) {
        if (QWidget* w = item->widget())
            w->deleteLater();
        delete item;
    }
    while (QLayoutItem* item = m_categoryChips->takeAt(0)) {
        if (QWidget* w = item->widget())
            w->deleteLater();
        delete item;
    }

    QHash<QString, int> countries;
    QHash<QString, int> categories;
    for (const Channel& c : m_all) {
        if (!c.country.trimmed().isEmpty())
            countries[c.country.trimmed()] += 1;
        if (!c.category().trimmed().isEmpty())
            categories[c.category().trimmed()] += 1;
    }

    const bool anyCountry = !countries.isEmpty();
    const bool anyCategory = !categories.isEmpty();
    m_countriesTitle->setVisible(anyCountry);
    m_categoriesTitle->setVisible(anyCategory);

    // Sort by popularity, take the top ones.
    QStringList countryKeys = countries.keys();
    std::sort(countryKeys.begin(), countryKeys.end(),
              [&countries](const QString& a, const QString& b) {
                  return countries.value(a) > countries.value(b);
              });
    QStringList categoryKeys = categories.keys();
    std::sort(categoryKeys.begin(), categoryKeys.end(),
              [&categories](const QString& a, const QString& b) {
                  return categories.value(a) > categories.value(b);
              });

    const auto addChips = [this](FlowLayout* layout, const QStringList& keys,
                                 const QHash<QString, int>& counts,
                                 const std::function<void(const QString&)>& onPick) {
        const int n = qMin(kMaxShortcutChips, keys.size());
        for (int i = 0; i < n; ++i) {
            const QString key = keys.at(i);
            auto* chip = new ChipButton(StringUtils::humanize(key)
                                        + QStringLiteral("   ") + QString::number(counts.value(key)));
            connect(chip, &QPushButton::clicked, this, [onPick, key]() { onPick(key); });
            layout->addWidget(chip);
        }
        if (keys.size() > kMaxShortcutChips) {
            auto* more = new ChipButton(QStringLiteral("View all"));
            connect(more, &QPushButton::clicked, this, [this]() { emit openPlaylistsRequested(); });
            layout->addWidget(more);
        }
    };

    addChips(m_countryChips, countryKeys, countries, [this](const QString& v) {
        emit countryShortcutSelected(v);
    });
    addChips(m_categoryChips, categoryKeys, categories, [this](const QString& v) {
        emit categoryShortcutSelected(v);
    });
}

// ---------------------------------------------------------------------------
// Dojo decoration: a faint seigaiha wave field plus two large, very subtle
// enso rings. Everything is drawn at low alpha so it stays background texture.
// ---------------------------------------------------------------------------
void HomePage::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    const qreal dpr = devicePixelRatioF();
    const QSize pixSize(qCeil(size().width() * dpr), qCeil(size().height() * dpr));
    if (m_decor.isNull() || m_decor.size() != pixSize) {
        m_decor = QPixmap(pixSize);
        m_decor.setDevicePixelRatio(dpr);
        m_decor.fill(Qt::transparent);
        QPainter p(&m_decor);
        drawDecoration(&p, size());
        p.end();
    }
    QPainter w(this);
    w.drawPixmap(QPointF(0.0, 0.0), m_decor);
}

void HomePage::drawDecoration(QPainter* p, const QSize& pageSize)
{
    p->setRenderHint(QPainter::Antialiasing);
    p->setBrush(Qt::NoBrush);

    // Seigaiha (wave) field: rows of semicircle outlines, every other row
    // offset by half a scale, so the arcs nest like fish scales.
    QColor wave = Theme::colors().accent;
    wave.setAlpha(6);
    QPen wavePen(wave, 2);
    wavePen.setCapStyle(Qt::RoundCap);
    p->setPen(wavePen);

    const qreal d = 72.0;     // wave scale (diameter)
    const qreal r = d / 2.0;  // vertical row spacing
    const int rows = qCeil(pageSize.height() / r) + 1;
    for (int row = 0; row < rows; ++row) {
        const qreal y = row * r;
        const qreal off = (row % 2 == 0) ? 0.0 : r;
        for (qreal x = off - r; x < pageSize.width() + r; x += d)
            p->drawArc(QRectF(x, y, d, d), 0, 180 * 16);
    }

    // Faint enso ring behind the hero title (top-left area).
    QColor enso = Theme::colors().accent;
    enso.setAlpha(12);
    QPen ensoPen(enso, 3);
    ensoPen.setCapStyle(Qt::RoundCap);
    p->setPen(ensoPen);
    const qreal r1 = qMax<qreal>(150.0, qMin(pageSize.width(), pageSize.height()) * 0.22);
    p->drawEllipse(QPointF(pageSize.width() * 0.20, r1 * 0.95), r1, r1);

    // Even fainter second ring, bottom-right, mostly clipped by the edge.
    QColor enso2 = Theme::colors().accent;
    enso2.setAlpha(7);
    QPen ensoPen2(enso2, 3);
    ensoPen2.setCapStyle(Qt::RoundCap);
    p->setPen(ensoPen2);
    const qreal r2 = qMax<qreal>(170.0, qMin(pageSize.width(), pageSize.height()) * 0.32);
    p->drawEllipse(QPointF(pageSize.width() - r2 * 0.40, pageSize.height() - r2 * 0.30), r2, r2);
}

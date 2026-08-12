#pragma once

#include <QPixmap>
#include <QWidget>

#include "models/Channel.h"

class ChannelView;
class FlowLayout;
class QLabel;
class QPainter;
class QPaintEvent;
class QVBoxLayout;

// ---------------------------------------------------------------------------
// HomePage - Continue Watching, Favorites and dynamically generated country
// and category shortcuts (never a hardcoded list).
// ---------------------------------------------------------------------------
class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget* parent = nullptr);

    void setAllChannels(const QVector<Channel>& channels);
    void setFavorites(const QVector<Channel>& favorites);
    void setHistory(const QVector<Channel>& history);
    void setCurrentKey(const QString& key);
    void refreshShortcuts();

signals:
    void channelActivated(const Channel& channel, const QVector<Channel>& sourcePool);
    void countryShortcutSelected(const QString& country);
    void categoryShortcutSelected(const QString& category);
    void openPlaylistsRequested();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* buildSectionTitle(const QString& text);
    void rebuildChips();
    // Paints the subtle dojo decoration (seigaiha wave field + faint enso
    // rings) behind the page content. Rendered once per size into a cached
    // pixmap so resize repaints stay cheap.
    void drawDecoration(QPainter* p, const QSize& pageSize);

    QVector<Channel> m_all;
    ChannelView* m_continueView = nullptr;
    ChannelView* m_favoritesView = nullptr;
    QLabel* m_continueTitle = nullptr;
    QLabel* m_favoritesTitle = nullptr;
    QLabel* m_countriesTitle = nullptr;
    QLabel* m_categoriesTitle = nullptr;
    FlowLayout* m_countryChips = nullptr;
    FlowLayout* m_categoryChips = nullptr;
    QVBoxLayout* m_rootLayout = nullptr;
    QPixmap m_decor; // cached decoration, keyed by device-pixel size
};

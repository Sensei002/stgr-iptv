#include "ui/pages/LiveTvPage.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

#include "services/FavoritesStore.h"
#include "settings/Settings.h"
#include "ui/ChannelView.h"
#include "ui/PlayerPanel.h"
#include "ui/Theme.h"

LiveTvPage::LiveTvPage(PlaybackController* controller, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(10);

    // --- toolbar (wrapped in a widget so fullscreen can hide the row) --------
    m_toolbarHost = new QWidget(this);
    auto* toolbar = new QHBoxLayout(m_toolbarHost);
    toolbar->setContentsMargins(0, 0, 0, 0);
    toolbar->setSpacing(8);

    auto* title = new QLabel(tr("Live TV"), m_toolbarHost);
    title->setProperty("stgrClass", QStringLiteral("pageTitle"));

    m_sortBox = new QComboBox(m_toolbarHost);
    m_sortBox->addItem(tr("Sort: Name"), QStringLiteral("name"));
    m_sortBox->addItem(tr("Sort: Country"), QStringLiteral("country"));
    m_sortBox->addItem(tr("Sort: Category"), QStringLiteral("category"));
    m_sortBox->addItem(tr("Sort: Channel number"), QStringLiteral("number"));
    connect(m_sortBox, &QComboBox::currentIndexChanged, this, [this](int) { applySortAndFilter(); });

    m_categoryBox = new QComboBox(m_toolbarHost);
    m_categoryBox->setMinimumWidth(180);
    connect(m_categoryBox, &QComboBox::currentIndexChanged, this, [this](int) { applySortAndFilter(); });

    m_refreshButton = new QToolButton(m_toolbarHost);
    m_refreshButton->setIcon(Theme::icon(QStringLiteral("refresh"), Theme::colors().text, 20));
    m_refreshButton->setToolTip(tr("Refresh all playlists"));
    m_refreshButton->setAutoRaise(true);

    toolbar->addWidget(title);
    toolbar->addStretch(1);
    toolbar->addWidget(m_sortBox);
    toolbar->addWidget(m_categoryBox);
    toolbar->addWidget(m_refreshButton);
    root->addWidget(m_toolbarHost);

    // --- splitter: channel list | player --------------------------------------
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);

    m_listHost = new QWidget(m_splitter);
    auto* listLayout = new QVBoxLayout(m_listHost);
    listLayout->setContentsMargins(0, 0, 0, 0);
    m_view = new ChannelView(m_listHost);
    m_view->setEmptyState(tr("No channels yet"),
                          tr("Add a playlist in Settings \u2192 Playlists, then refresh it."));
    listLayout->addWidget(m_view);

    m_player = new PlayerPanel(m_controller, m_splitter);

    m_splitter->addWidget(m_listHost);
    m_splitter->addWidget(m_player);
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 2);
    m_splitter->setSizes({ 520, 560 });
    root->addWidget(m_splitter, 1);

    // Channel list is always List view (no grid toggle).
    m_view->setGridMode(false);

    connect(m_view, &ChannelView::channelActivated,
            this, [this](const Channel& ch) {
                emit channelActivated(ch, m_visible);
            });
    connect(m_refreshButton, &QToolButton::clicked,
            this, &LiveTvPage::refreshRequested);

    connect(m_view, &ChannelView::favoriteToggled,
            this, [this](const Channel&, bool) {
                setFavoriteKeys(FavoritesStore::instance()->keys());
            });
}

void LiveTvPage::setAllChannels(const QVector<Channel>& channels)
{
    m_all = channels;
    rebuildCategoryFilter();
    applySortAndFilter();
}

void LiveTvPage::rebuildCategoryFilter()
{
    const QString current = m_categoryBox->currentData().toString();
    m_categoryBox->blockSignals(true);
    m_categoryBox->clear();
    m_categoryBox->addItem(tr("All categories"), QString());

    QSet<QString> seen;
    for (const Channel& c : m_all) {
        const QString cat = c.category().trimmed();
        if (!cat.isEmpty() && !seen.contains(cat)) {
            seen.insert(cat);
            m_categoryBox->addItem(cat, cat);
        }
    }
    m_categoryBox->blockSignals(false);

    const int idx = m_categoryBox->findData(current);
    m_categoryBox->setCurrentIndex(idx >= 0 ? idx : 0);
}

void LiveTvPage::applySortAndFilter()
{
    QVector<Channel> out = m_all;

    const QString category = m_categoryBox->currentData().toString();
    if (!category.isEmpty()) {
        out.erase(std::remove_if(out.begin(), out.end(),
                                 [&category](const Channel& c) { return c.category().trimmed() != category; }),
                  out.end());
    }

    const QString sort = m_sortBox->currentData().toString();
    auto cmpName = [](const Channel& a, const Channel& b) {
        return QString::compare(a.lowerName(), b.lowerName(), Qt::CaseInsensitive) < 0;
    };
    if (sort == QLatin1String("country")) {
        std::stable_sort(out.begin(), out.end(), [](const Channel& a, const Channel& b) {
            const int r = QString::compare(a.country, b.country, Qt::CaseInsensitive);
            return r != 0 ? r < 0 : QString::compare(a.lowerName(), b.lowerName(), Qt::CaseInsensitive) < 0;
        });
    } else if (sort == QLatin1String("category")) {
        std::stable_sort(out.begin(), out.end(), [](const Channel& a, const Channel& b) {
            const int r = QString::compare(a.category(), b.category(), Qt::CaseInsensitive);
            return r != 0 ? r < 0 : QString::compare(a.lowerName(), b.lowerName(), Qt::CaseInsensitive) < 0;
        });
    } else if (sort == QLatin1String("number")) {
        std::stable_sort(out.begin(), out.end(), [](const Channel& a, const Channel& b) {
            return a.number < b.number;
        });
    } else {
        std::stable_sort(out.begin(), out.end(), cmpName);
    }

    m_visible = out;
    m_view->setChannels(out);
    m_view->setCurrentKey(m_currentKey);
}

void LiveTvPage::setCurrentKey(const QString& key)
{
    m_currentKey = key;
    m_view->setCurrentKey(key);
}

void LiveTvPage::setFavoriteKeys(const QSet<QString>& keys)
{
    m_view->setFavoriteKeys(keys);
}

void LiveTvPage::setShowLogos(bool show)
{
    m_view->setShowLogos(show);
}

void LiveTvPage::applySettings()
{
    m_view->setShowLogos(Settings::instance()->showLogos());
}

void LiveTvPage::setPlayerExpanded(bool expanded)
{
    // Hide the toolbar and the channel list so the player fills the whole
    // page; restore them (and the original proportions) when leaving.
    m_toolbarHost->setVisible(!expanded);
    m_listHost->setVisible(!expanded);
    if (auto* root = qobject_cast<QVBoxLayout*>(layout())) {
        root->setContentsMargins(expanded ? 0 : 16, expanded ? 0 : 14,
                                 expanded ? 0 : 16, expanded ? 0 : 14);
        root->setSpacing(expanded ? 0 : 10);
    }
    if (expanded) {
        m_splitter->setStretchFactor(0, 0);
        m_splitter->setStretchFactor(1, 1);
    } else {
        m_splitter->setStretchFactor(0, 3);
        m_splitter->setStretchFactor(1, 2);
        m_splitter->setSizes({ 520, 560 });
    }
}

void LiveTvPage::focusChannelList()
{
    m_view->setFocus();
}

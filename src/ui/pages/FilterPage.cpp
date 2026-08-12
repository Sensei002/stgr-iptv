#include "ui/pages/FilterPage.h"

#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>

#include "ui/ChannelView.h"
#include "ui/ChipButton.h"
#include "ui/FlowLayout.h"
#include "utils/StringUtils.h"

namespace {
QString valueOf(const Channel& c, FilterPage::Kind kind)
{
    switch (kind) {
    case FilterPage::Kind::Countries:   return c.country.trimmed();
    case FilterPage::Kind::Categories:  return c.category().trimmed();
    case FilterPage::Kind::Languages:   return c.language.trimmed();
    }
    return {};
}
}

FilterPage::FilterPage(Kind kind, QWidget* parent)
    : QWidget(parent)
    , m_kind(kind)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(10);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setProperty("stgrClass", QStringLiteral("pageTitle"));
    root->addWidget(m_titleLabel);

    // Chips scroll area
    auto* chipHost = new QWidget(this);
    m_chipsLayout = new FlowLayout(chipHost, 0, 8, 8);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setMaximumHeight(96);
    scroll->setWidget(chipHost);
    root->addWidget(scroll);

    m_view = new ChannelView(this);
    m_view->setEmptyState(QString(), tr("Select a filter above to browse channels."));
    root->addWidget(m_view, 1);

    connect(m_view, &ChannelView::channelActivated,
            this, [this](const Channel& ch) {
                emit channelActivated(ch, m_view->channels());
            });
}

void FilterPage::setAllChannels(const QVector<Channel>& channels)
{
    m_all = channels;

    switch (m_kind) {
    case Kind::Countries:
        m_titleLabel->setText(tr("Countries"));
        break;
    case Kind::Categories:
        m_titleLabel->setText(tr("Categories"));
        break;
    case Kind::Languages:
        m_titleLabel->setText(tr("Languages"));
        break;
    }

    rebuildChips();
    applyFilter();
}

void FilterPage::selectValue(const QString& value)
{
    m_selected = value;
    for (auto it = m_chipValues.begin(); it != m_chipValues.end(); ++it) {
        auto* chip = qobject_cast<ChipButton*>(it.key());
        if (chip)
            chip->setSelected(it.value() == m_selected);
    }
    applyFilter();
}

void FilterPage::setCurrentKey(const QString& key)
{
    m_view->setCurrentKey(key);
}

void FilterPage::rebuildChips()
{
    while (QLayoutItem* item = m_chipsLayout->takeAt(0)) {
        if (QWidget* w = item->widget())
            w->deleteLater();
        delete item;
    }
    m_chipValues.clear();
    m_counts.clear();

    for (const Channel& c : m_all) {
        const QString v = valueOf(c, m_kind);
        if (!v.isEmpty())
            m_counts[v] += 1;
    }

    auto* allChip = new ChipButton(tr("All"));
    m_chipValues.insert(allChip, QString());
    connect(allChip, &QPushButton::clicked, this, [this]() { selectValue(QString()); });
    m_chipsLayout->addWidget(allChip);

    QStringList keys = m_counts.keys();
    std::sort(keys.begin(), keys.end(),
              [this](const QString& a, const QString& b) {
                  return m_counts.value(a) > m_counts.value(b);
              });

    for (const QString& key : keys) {
        auto* chip = new ChipButton(StringUtils::humanize(key)
                                    + QStringLiteral("   ") + QString::number(m_counts.value(key)));
        m_chipValues.insert(chip, key);
        connect(chip, &QPushButton::clicked, this, [this, key]() { selectValue(key); });
        m_chipsLayout->addWidget(chip);
    }

    m_selected.clear();
    allChip->setSelected(true);
}

void FilterPage::applyFilter()
{
    if (m_selected.isEmpty()) {
        m_view->setChannels(m_all);
        m_view->setEmptyState(tr("No channels"),
                              tr("Add and refresh a playlist to see channels here."));
        return;
    }

    QVector<Channel> filtered;
    for (const Channel& c : m_all) {
        if (valueOf(c, m_kind) == m_selected)
            filtered.append(c);
    }
    m_view->setChannels(filtered);
    m_view->setEmptyState(tr("Nothing here yet"),
                          tr("No channels match this selection."));
}

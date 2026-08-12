#pragma once

#include <QHash>
#include <QWidget>

#include "models/Channel.h"

class ChannelView;
class ChipButton;
class FlowLayout;
class QLabel;
class QPushButton;

// ---------------------------------------------------------------------------
// FilterPage - the shared Countries / Categories / Languages page.
//
// Shows a scrollable chip list (built dynamically from the channel metadata,
// never hardcoded) above a virtualized channel grid filtered by the selected
// chip.
// ---------------------------------------------------------------------------
class FilterPage : public QWidget
{
    Q_OBJECT

public:
    enum class Kind { Countries, Categories, Languages };

    explicit FilterPage(Kind kind, QWidget* parent = nullptr);

    void setAllChannels(const QVector<Channel>& channels);
    void selectValue(const QString& value); // activate a chip externally
    void setCurrentKey(const QString& key);

    Kind kind() const { return m_kind; }

signals:
    void channelActivated(const Channel& channel, const QVector<Channel>& sourcePool);

private:
    void rebuildChips();
    void applyFilter();

    Kind m_kind;
    QVector<Channel> m_all;
    QHash<QString, int> m_counts;
    QString m_selected;

    FlowLayout* m_chipsLayout = nullptr;
    ChannelView* m_view = nullptr;
    QLabel* m_titleLabel = nullptr;
    QHash<QPushButton*, QString> m_chipValues; // chip -> filter value
};

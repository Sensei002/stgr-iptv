#pragma once

#include <QPushButton>

// ---------------------------------------------------------------------------
// ChipButton - a pill-shaped selectable chip used by the filter pages and
// the Home screen shortcuts (countries, categories, languages).
// ---------------------------------------------------------------------------
class ChipButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ChipButton(const QString& text, const QString& countText = QString(),
                        QWidget* parent = nullptr);

    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }

private:
    void updateStyle();

    bool m_selected = false;
    QString m_countText;
};

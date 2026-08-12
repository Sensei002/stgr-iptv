#include "ui/ChipButton.h"

#include "ui/Theme.h"

ChipButton::ChipButton(const QString& text, const QString& countText, QWidget* parent)
    : QPushButton(text, parent)
    , m_countText(countText)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    updateStyle();
}

void ChipButton::setSelected(bool selected)
{
    if (m_selected == selected)
        return;
    m_selected = selected;
    updateStyle();
}

void ChipButton::updateStyle()
{
    const Theme::Colors& c = Theme::colors();
    QString style;
    if (m_selected) {
        style = QStringLiteral(
                    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: 15px;"
                    " padding: 5px 13px; font-weight: 600; }")
                    .arg(c.accentSoft.name(), c.accentHover.name(), c.accent.name());
    } else {
        style = QStringLiteral(
                    "QPushButton { background: #121216; color: %1; border: 1px solid %2; border-radius: 15px;"
                    " padding: 5px 13px; }"
                    "QPushButton:hover { border-color: %3; color: %4; }")
                    .arg(c.text.name(), c.border.name(), c.accent.name(), c.accentHover.name());
    }

    setStyleSheet(style);
}

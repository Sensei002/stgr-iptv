#pragma once

#include <QColor>
#include <QIcon>
#include <QPalette>
#include <QString>

// ---------------------------------------------------------------------------
// Theme - STGR IpTV's visual identity.
//
// Japanese dojo + modern tech: near-black surfaces, deep crimson accents and
// subtle gold highlights. Icons are drawn programmatically (no binary assets
// to ship) and follow the palette.
// ---------------------------------------------------------------------------
namespace Theme {

struct Colors {
    QColor bg;          // near-black base
    QColor panel;       // dark charcoal panels
    QColor panelAlt;    // elevated surface
    QColor border;      // hairline borders
    QColor text;        // primary text
    QColor textDim;     // secondary text
    QColor accent;      // crimson
    QColor accentHover;
    QColor accentPressed;
    QColor accentSoft;  // tinted background for active states
    QColor gold;
    QColor goldDim;
    QColor success;
    QColor warning;
    QColor error;
};

const Colors& colors();

// Full application stylesheet (dark theme, tuned for QWidgets).
QString stylesheet();

QPalette palette();

// Programmatically drawn icons (24px logical by default).
QIcon icon(const QString& name, const QColor& color = QColor(), int size = 24);

// Renders the STGR logo from the bundled SVG at the requested size.
QIcon appIcon();

} // namespace Theme

#include "ui/Theme.h"

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QSvgRenderer>
#include <QtMath>
#include <QWidget>

namespace {
constexpr qreal kPi = 3.14159265358979323846;
} // namespace

namespace Theme {

Colors g_colors;

const Colors& colors()
{
    if (!g_colors.bg.isValid()) {
        g_colors.bg = QColor(QStringLiteral("#0d0d10"));
        g_colors.panel = QColor(QStringLiteral("#15151a"));
        g_colors.panelAlt = QColor(QStringLiteral("#1b1b22"));
        g_colors.border = QColor(QStringLiteral("#26262f"));
        g_colors.text = QColor(QStringLiteral("#e9e9ec"));
        g_colors.textDim = QColor(QStringLiteral("#999aa4"));
        g_colors.accent = QColor(QStringLiteral("#e2343f"));
        g_colors.accentHover = QColor(QStringLiteral("#ef4651"));
        g_colors.accentPressed = QColor(QStringLiteral("#c22430"));
        g_colors.accentSoft = QColor(QStringLiteral("#3a141a"));
        g_colors.gold = QColor(QStringLiteral("#d9b64a"));
        g_colors.goldDim = QColor(QStringLiteral("#a8862e"));
        g_colors.success = QColor(QStringLiteral("#3ecf8e"));
        g_colors.warning = QColor(QStringLiteral("#e0a030"));
        g_colors.error = QColor(QStringLiteral("#e2343f"));
    }
    return g_colors;
}

QPalette palette()
{
    const Colors& c = colors();
    QPalette p;
    p.setColor(QPalette::Window, c.bg);
    p.setColor(QPalette::WindowText, c.text);
    p.setColor(QPalette::Base, c.panel);
    p.setColor(QPalette::AlternateBase, c.panelAlt);
    p.setColor(QPalette::Text, c.text);
    p.setColor(QPalette::Button, c.panelAlt);
    p.setColor(QPalette::ButtonText, c.text);
    p.setColor(QPalette::Highlight, c.accent);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase, c.panelAlt);
    p.setColor(QPalette::ToolTipText, c.text);
    p.setColor(QPalette::PlaceholderText, c.textDim);
    p.setColor(QPalette::Disabled, QPalette::Text, c.textDim);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, c.textDim);
    p.setColor(QPalette::Link, c.gold);
    return p;
}

QString stylesheet()
{
    return QStringLiteral(R"QSS(
* { outline: none; }
QWidget { color: #e9e9ec; font-family: "Segoe UI"; font-size: 13px; }
QMainWindow { background: #0d0d10; }

QFrame[stgrClass="panel"] { background: #15151a; border: 1px solid #26262f; border-radius: 8px; }
QFrame[stgrClass="card"]  { background: #1b1b22; border: 1px solid #26262f; border-radius: 10px; }
QFrame[stgrClass="topBar"] { background: #121216; border-bottom: 1px solid #26262f; }

QListWidget#sidebar { background: #121216; border: none; border-right: 1px solid #26262f; outline: 0; padding: 10px 4px; }
QListWidget#sidebar::item { height: 38px; border-radius: 8px; padding-left: 10px; margin: 2px 4px; color: #999aa4; }
QListWidget#sidebar::item:hover { background: #1b1b22; color: #e9e9ec; }
QListWidget#sidebar::item:selected { background: #3a141a; color: #ef4651; border-left: 3px solid #e2343f; padding-left: 7px; }

QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    background: #121216; border: 1px solid #2c2c36; border-radius: 6px;
    padding: 6px 10px; color: #e9e9ec; selection-background-color: #e2343f; selection-color: white;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border: 1px solid #e2343f; }
QLineEdit[search="true"] { border-radius: 18px; padding: 7px 16px; }
QComboBox::drop-down { border: none; width: 26px; }
QComboBox::down-arrow { image: none; border-left: 4px solid transparent; border-right: 4px solid transparent; border-top: 5px solid #999aa4; margin-right: 8px; }
QComboBox QAbstractItemView { background: #1b1b22; border: 1px solid #2c2c36; border-radius: 6px; selection-background-color: #3a141a; selection-color: #e9e9ec; }

QPushButton { background: #26262f; color: #e9e9ec; border: 1px solid #32323d; border-radius: 6px; padding: 7px 14px; }
QPushButton:hover { background: #2f2f3a; }
QPushButton:pressed { background: #23232b; }
QPushButton[accent="true"] { background: #e2343f; border-color: #e2343f; color: #ffffff; font-weight: 600; }
QPushButton[accent="true"]:hover { background: #ef4651; }
QPushButton[accent="true"]:pressed { background: #c22430; }
QPushButton[flat="true"] { background: transparent; border: none; }
QPushButton[flat="true"]:hover { background: #1b1b22; }
QPushButton:disabled { color: #5a5a66; background: #1b1b22; border-color: #26262f; }

QToolButton { background: transparent; border: none; border-radius: 6px; color: #e9e9ec; padding: 4px; }
QToolButton:hover { background: #26262f; }
QToolButton:checked { background: #3a141a; color: #ef4651; }
QToolButton:disabled { color: #4a4a56; }

QSlider::groove:horizontal { height: 4px; background: #2c2c36; border-radius: 2px; }
QSlider::sub-page:horizontal { background: #e2343f; border-radius: 2px; }
QSlider::handle:horizontal { width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; background: #e9e9ec; }
QSlider::handle:horizontal:hover { background: #ffffff; }

QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: #2c2c36; border-radius: 5px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: #3a3a46; }
QScrollBar:add-line:vertical, QScrollBar:sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle:horizontal { background: #2c2c36; border-radius: 5px; min-width: 30px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

QListView { background: transparent; border: none; }
QListView::item { background: transparent; }
QAbstractScrollArea::corner { background: transparent; }

QSplitter::handle { background: transparent; }
QSplitter::handle:horizontal { width: 6px; }

QTabWidget::pane { border: 1px solid #26262f; border-radius: 8px; top: -1px; background: #15151a; }
QTabBar::tab { background: transparent; color: #999aa4; padding: 8px 16px; border-bottom: 2px solid transparent; }
QTabBar::tab:selected { color: #ef4651; border-bottom: 2px solid #e2343f; }
QTabBar::tab:hover { color: #e9e9ec; }

QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #3a3a46; border-radius: 4px; background: #121216; }
QCheckBox::indicator:hover { border-color: #e2343f; }
QCheckBox::indicator:checked { background: #e2343f; border-color: #e2343f; }

QMenu { background: #1b1b22; border: 1px solid #2c2c36; border-radius: 8px; padding: 6px; }
QMenu::item { padding: 6px 24px; border-radius: 5px; }
QMenu::item:selected { background: #3a141a; color: #ef4651; }
QMenu::separator { height: 1px; background: #26262f; margin: 4px 8px; }

QToolTip { background: #1b1b22; color: #e9e9ec; border: 1px solid #2c2c36; padding: 4px 8px; }

QDialog { background: #15151a; }
QMessageBox { background: #15151a; }

QProgressBar { background: #1b1b22; border: none; border-radius: 3px; height: 6px; }
QProgressBar::chunk { background: #e2343f; border-radius: 3px; }

QLabel[stgrClass="pageTitle"] { font-size: 22px; font-weight: 600; color: #e9e9ec; }
QLabel[stgrClass="sectionTitle"] { font-size: 12px; font-weight: 700; color: #8a8a96; }
QLabel[stgrClass="dim"] { color: #999aa4; }
QLabel[stgrClass="gold"] { color: #d9b64a; }
QLabel[stgrClass="accent"] { color: #ef4651; }
)QSS");
}

namespace {

QPen pen(const QColor& color, qreal width)
{
    QPen p(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    return p;
}

QIcon drawIcon(const QString& name, const QColor& color, int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal s = size;
    const qreal sw = qMax<qreal>(1.5, s * 0.085);
    const QColor c = color.isValid() ? color : colors().text;

    auto stroke = [&p, &c, sw]() { p.setPen(pen(c, sw)); p.setBrush(Qt::NoBrush); };
    auto fill = [&p, &c]() { p.setPen(Qt::NoPen); p.setBrush(c); };

    auto centerRect = [&s](qreal x, qreal y, qreal w, qreal h) {
        return QRectF(x * s / 24.0, y * s / 24.0, w * s / 24.0, h * s / 24.0);
    };
    auto pt = [&s](qreal x, qreal y) { return QPointF(x * s / 24.0, y * s / 24.0); };

    if (name == QLatin1String("home")) {
        QPainterPath path;
        path.moveTo(pt(4, 12));
        path.lineTo(pt(12, 5));
        path.lineTo(pt(20, 12));
        path.lineTo(pt(20, 19));
        path.lineTo(pt(15, 19));
        path.lineTo(pt(15, 14));
        path.lineTo(pt(9, 14));
        path.lineTo(pt(9, 19));
        path.lineTo(pt(4, 19));
        path.closeSubpath();
        fill(); p.drawPath(path);
    } else if (name == QLatin1String("tv")) {
        stroke(); p.drawRoundedRect(centerRect(3, 6, 18, 13), 1.5, 1.5);
        p.drawLine(pt(12, 6), pt(12, 3));
        p.drawLine(pt(9, 2.5), pt(15, 2.5));
    } else if (name == QLatin1String("star") || name == QLatin1String("star-outline")) {
        QPainterPath path;
        const QPointF center = pt(12, 13);
        for (int i = 0; i < 10; ++i) {
            const qreal r = (i % 2 == 0) ? s * 0.42 : s * 0.19;
            const qreal a = -kPi / 2.0 + i * kPi / 5.0;
            const QPointF pnt(center.x() + r * qCos(a), center.y() + r * qSin(a));
            if (i == 0) path.moveTo(pnt); else path.lineTo(pnt);
        }
        path.closeSubpath();
        if (name == QLatin1String("star")) { fill(); p.drawPath(path); }
        else { stroke(); p.drawPath(path); }
    } else if (name == QLatin1String("history")) {
        stroke();
        p.drawEllipse(pt(12, 13), s * 0.36, s * 0.36);
        p.drawLine(pt(12, 13), pt(12, 9));
        p.drawLine(pt(12, 13), pt(15.5, 14.5));
    } else if (name == QLatin1String("globe")) {
        stroke();
        p.drawEllipse(pt(12, 12), s * 0.38, s * 0.38);
        p.drawLine(pt(12, 3.5), pt(12, 20.5));
        p.drawEllipse(pt(12, 12), s * 0.30, s * 0.42);
        p.drawLine(pt(4.5, 9.5), pt(19.5, 9.5));
        p.drawLine(pt(4.5, 14.5), pt(19.5, 14.5));
    } else if (name == QLatin1String("grid")) {
        stroke();
        p.drawRoundedRect(centerRect(4, 4, 7.2, 7.2), 1, 1);
        p.drawRoundedRect(centerRect(12.8, 4, 7.2, 7.2), 1, 1);
        p.drawRoundedRect(centerRect(4, 12.8, 7.2, 7.2), 1, 1);
        p.drawRoundedRect(centerRect(12.8, 12.8, 7.2, 7.2), 1, 1);
    } else if (name == QLatin1String("language")) {
        QFont f(QStringLiteral("Segoe UI"), 9);
        f.setBold(true);
        p.setFont(f);
        p.setPen(colors().accent);
        p.drawText(QRectF(0, 2 * s / 24.0, s, s), Qt::AlignCenter, QStringLiteral("A\u3082"));
    } else if (name == QLatin1String("settings")) {
        stroke();
        p.drawEllipse(pt(12, 12), s * 0.16, s * 0.16);
        for (int i = 0; i < 8; ++i) {
            const qreal a = i * kPi / 4.0;
            p.drawLine(pt(12 + 7.5 * qCos(a), 12 + 7.5 * qSin(a)),
                       pt(12 + 10.0 * qCos(a), 12 + 10.0 * qSin(a)));
        }
    } else if (name == QLatin1String("info")) {
        stroke();
        p.drawEllipse(pt(12, 12), s * 0.36, s * 0.36);
        p.drawLine(pt(12, 9.5), pt(12, 10.5));
        p.drawLine(pt(12, 12.5), pt(12, 16.5));
    } else if (name == QLatin1String("search")) {
        stroke();
        p.drawEllipse(pt(10.5, 10.5), s * 0.26, s * 0.26);
        p.drawLine(pt(14.5, 14.5), pt(20, 20));
    } else if (name == QLatin1String("play")) {
        QPainterPath path;
        path.moveTo(pt(8.5, 6));
        path.lineTo(pt(8.5, 18));
        path.lineTo(pt(18, 12));
        path.closeSubpath();
        fill(); p.drawPath(path);
    } else if (name == QLatin1String("pause")) {
        fill();
        p.drawRect(centerRect(8, 6, 2.8, 12));
        p.drawRect(centerRect(13.2, 6, 2.8, 12));
    } else if (name == QLatin1String("stop")) {
        fill();
        p.drawRect(centerRect(7, 7, 10, 10));
    } else if (name == QLatin1String("prev") || name == QLatin1String("next")) {
        const bool isNext = name == QLatin1String("next");
        QPainterPath path;
        if (isNext) {
            path.moveTo(pt(16.5, 6.5));
            path.lineTo(pt(16.5, 17.5));
            path.lineTo(pt(9, 12));
        } else {
            path.moveTo(pt(7.5, 6.5));
            path.lineTo(pt(7.5, 17.5));
            path.lineTo(pt(15, 12));
        }
        path.closeSubpath();
        fill(); p.drawPath(path);
        fill();
        p.drawRect(isNext ? centerRect(6.2, 6.5, 1.8, 11) : centerRect(16, 6.5, 1.8, 11));
    } else if (name == QLatin1String("volume") || name == QLatin1String("mute")) {
        QPainterPath speaker;
        speaker.moveTo(pt(5, 9.5));
        speaker.lineTo(pt(8.5, 9.5));
        speaker.lineTo(pt(12.5, 6.5));
        speaker.lineTo(pt(12.5, 17.5));
        speaker.lineTo(pt(8.5, 14.5));
        speaker.lineTo(pt(5, 14.5));
        speaker.closeSubpath();
        fill(); p.drawPath(speaker);
        stroke();
        if (name == QLatin1String("volume")) {
            p.drawArc(centerRect(12, 9, 8, 6), -60 * 16, 120 * 16);
            p.drawArc(centerRect(13.5, 7.5, 11, 9), -60 * 16, 120 * 16);
        } else {
            p.drawLine(pt(15, 9), pt(20, 13));
            p.drawLine(pt(20, 9), pt(15, 13));
        }
    } else if (name == QLatin1String("fullscreen")) {
        stroke();
        p.drawLine(pt(5, 9), pt(5, 5));
        p.drawLine(pt(5, 5), pt(9, 5));
        p.drawLine(pt(15, 5), pt(19, 5));
        p.drawLine(pt(19, 5), pt(19, 9));
        p.drawLine(pt(19, 15), pt(19, 19));
        p.drawLine(pt(19, 19), pt(15, 19));
        p.drawLine(pt(9, 19), pt(5, 19));
        p.drawLine(pt(5, 19), pt(5, 15));
    } else if (name == QLatin1String("aspect")) {
        stroke();
        p.drawRect(centerRect(3, 6, 18, 12));
        p.drawRect(centerRect(7.5, 9.5, 9, 5));
    } else if (name == QLatin1String("refresh")) {
        stroke();
        QPainterPath arc;
        arc.moveTo(pt(15.5, 6.5));
        arc.arcTo(centerRect(5.5, 5.5, 13, 13), 45, 270);
        p.drawPath(arc);
        p.setBrush(c);
        p.drawPolygon(QPolygonF({
            pt(16.8, 5.6), pt(19.4, 7.4), pt(17.0, 8.6)
        }));
        p.drawPolygon(QPolygonF({
            pt(7.2, 18.4), pt(4.6, 16.6), pt(7.0, 15.4)
        }));
    } else if (name == QLatin1String("add")) {
        p.setPen(pen(c, sw * 1.4));
        p.drawLine(pt(12, 5), pt(12, 19));
        p.drawLine(pt(5, 12), pt(19, 12));
    } else if (name == QLatin1String("edit")) {
        stroke();
        p.drawLine(pt(5.5, 18.5), pt(6.5, 18.5));
        p.drawLine(pt(6.5, 18.5), pt(16.5, 8.5));
        p.drawLine(pt(16.5, 8.5), pt(19.5, 5.5));
        p.drawLine(pt(14.5, 10.5), pt(17.5, 13.5));
    } else if (name == QLatin1String("delete")) {
        stroke();
        p.drawRoundedRect(centerRect(8, 9, 8, 9), 1, 1);
        p.drawLine(pt(6, 7.5), pt(18, 7.5));
        p.drawLine(pt(10.5, 5.5), pt(13.5, 5.5));
        p.drawLine(pt(10.5, 11.5), pt(10.5, 16.5));
        p.drawLine(pt(13.5, 11.5), pt(13.5, 16.5));
    } else if (name == QLatin1String("folder")) {
        QPainterPath path;
        path.moveTo(pt(4, 7));
        path.lineTo(pt(9.5, 7));
        path.lineTo(pt(11.5, 9));
        path.lineTo(pt(20, 9));
        path.lineTo(pt(20, 18));
        path.lineTo(pt(4, 18));
        path.closeSubpath();
        stroke(); p.drawPath(path);
    } else if (name == QLatin1String("back")) {
        stroke();
        QPainterPath path;
        path.moveTo(pt(14.5, 6));
        path.lineTo(pt(8.5, 12));
        path.lineTo(pt(14.5, 18));
        p.drawPath(path);
    } else if (name == QLatin1String("right")) {
        stroke();
        QPainterPath path;
        path.moveTo(pt(9.5, 6));
        path.lineTo(pt(15.5, 12));
        path.lineTo(pt(9.5, 18));
        p.drawPath(path);
    } else if (name == QLatin1String("close")) {
        p.setPen(pen(c, sw * 1.2));
        p.drawLine(pt(7, 7), pt(17, 17));
        p.drawLine(pt(17, 7), pt(7, 17));
    } else if (name == QLatin1String("external")) {
        stroke();
        p.drawRect(centerRect(4, 4, 13, 13));
        p.drawLine(pt(4, 17), pt(10, 11));
        p.drawLine(pt(8, 8), pt(10, 8));
        p.drawLine(pt(8, 8), pt(8, 10));
        p.drawLine(pt(4, 12.5), pt(4, 4));
        p.drawLine(pt(12.5, 4), pt(4, 4));
    } else if (name == QLatin1String("heart")) {
        QPainterPath path;
        path.moveTo(pt(12, 19.5));
        path.cubicTo(pt(4, 15), pt(3.5, 8.5), pt(7, 6));
        path.cubicTo(pt(10, 4), pt(12, 6.5), pt(12, 8));
        path.cubicTo(pt(12, 6.5), pt(14, 4), pt(17, 6));
        path.cubicTo(pt(20.5, 8.5), pt(20, 15), pt(12, 19.5));
        fill(); p.drawPath(path);
    } else if (name == QLatin1String("list")) {
        stroke();
        for (int i = 0; i < 3; ++i) {
            p.drawLine(pt(4, 6 + i * 6), pt(9, 6 + i * 6));
            p.drawLine(pt(11, 6 + i * 6), pt(20, 6 + i * 6));
        }
    } else {
        // Fallback: a filled circle.
        fill();
        p.drawEllipse(pt(12, 12), s * 0.4, s * 0.4);
    }

    p.end();
    return QIcon(pm);
}

} // namespace

QIcon icon(const QString& name, const QColor& color, int size)
{
    return drawIcon(name, color, size);
}

QIcon appIcon()
{
    QIcon icon;
    QSvgRenderer renderer(QStringLiteral(":/branding/logo.svg"));
    if (renderer.isValid()) {
        const QList<int> sizes = { 16, 24, 32, 48, 64, 128, 256 };
        for (int s : sizes) {
            QPixmap pm(s, s);
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            // The artwork is not perfectly square; render centered and
            // aspect-preserved instead of stretching it into the viewport.
            const QSizeF vb = renderer.viewBoxF().size();
            const qreal scale = qMin<qreal>(qreal(s) / vb.width(), qreal(s) / vb.height());
            const QSizeF sz = vb * scale;
            const QRectF target((s - sz.width()) / 2.0, (s - sz.height()) / 2.0,
                                sz.width(), sz.height());
            renderer.render(&p, target);
            p.end();
            icon.addPixmap(pm);
        }
    } else {
        icon = drawIcon(QStringLiteral("tv"), colors().accent, 32);
    }
    return icon;
}

} // namespace Theme

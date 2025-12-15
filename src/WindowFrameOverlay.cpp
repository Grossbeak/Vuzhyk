#include "WindowFrameOverlay.h"

#include <QPainter>

WindowFrameOverlay::WindowFrameOverlay(QWidget *parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setFocusPolicy(Qt::NoFocus);
}

void WindowFrameOverlay::setRadius(int radius) {
    m_radius = radius;
    update();
}

void WindowFrameOverlay::setPenColor(const QColor &color) {
    m_color = color;
    update();
}

void WindowFrameOverlay::setPenWidth(int width) {
    m_width = width;
    update();
}

void WindowFrameOverlay::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(m_color, m_width);
    pen.setCosmetic(true);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    const qreal half = m_width / 2.0;
    QRectF r = rect().adjusted(half, half, -half, -half);
    if (m_radius > 0) {
        p.drawRoundedRect(r, m_radius, m_radius);
    } else {
        p.drawRect(r);
    }
}
























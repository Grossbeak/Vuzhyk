#pragma once

#include <QWidget>

class WindowFrameOverlay : public QWidget {
    Q_OBJECT
public:
    explicit WindowFrameOverlay(QWidget *parent = nullptr);

    void setRadius(int radius);
    void setPenColor(const QColor &color);
    void setPenWidth(int width);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_radius { 8 };
    QColor m_color { Qt::black };
    int m_width { 2 };
};




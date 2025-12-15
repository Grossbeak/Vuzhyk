#pragma once

#include <QWidget>
#include <QPointer>

class QMenuBar;
class QToolButton;
class QHBoxLayout;
class QMainWindow;
class QIcon;
class QLabel;
class QPushButton;

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QMainWindow *window, QMenuBar *menuBar, QWidget *parent = nullptr);
    void updateIconsForTheme(const QString &theme);

signals:
    void settingsClicked();
    void iconTripleClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onMinimize();
    void onMaximizeRestore();
    void onClose();

private:
    void updateMaximizeIcon();
    QIcon createThemedIcon(const QString &resourcePath, const QString &color);

    QPointer<QMainWindow> m_window;
    QMenuBar *m_menuBar { nullptr };
    QToolButton *m_minButton { nullptr };
    QToolButton *m_maxButton { nullptr };
    QToolButton *m_closeButton { nullptr };
    QLabel *m_appIconLabel { nullptr };
    QPushButton *m_appIconButton { nullptr }; // Альтернативный вариант - кнопка вместо label
    QToolButton *m_settingsButton { nullptr };
    QHBoxLayout *m_layout { nullptr };
    QPoint m_dragOffset;

    // icons
    QIcon *m_minIcon { nullptr };
    QIcon *m_minIconHover { nullptr };
    QIcon *m_maxIcon { nullptr };
    QIcon *m_maxIconHover { nullptr };
    QIcon *m_restoreIcon { nullptr };
    QIcon *m_restoreIconHover { nullptr };
    QIcon *m_closeIcon { nullptr };
    QIcon *m_closeIconHover { nullptr };
    QIcon *m_settingsIcon { nullptr };
    QIcon *m_settingsIconHover { nullptr };
    
    // Для отслеживания восьмерного клика
    qint64 m_lastClickTime;
    int m_clickCount;
    QPoint m_lastClickPos;
};



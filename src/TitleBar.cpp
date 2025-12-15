#include "TitleBar.h"

#include <QMenuBar>
#include <QToolButton>
#include <QHBoxLayout>
#include <QStyle>
#include <QMouseEvent>
#include <QMainWindow>
#include <QLabel>
#include <QPixmap>
#include <QFile>
#include <QSvgRenderer>
#include <QPainter>
#include <QByteArray>
#include <QElapsedTimer>
#include <QApplication>
#include <QPushButton>
#include <QDateTime>

TitleBar::TitleBar(QMainWindow *window, QMenuBar *menuBar, QWidget *parent)
    : QWidget(parent), m_window(window), m_menuBar(menuBar)
    , m_lastClickTime(0), m_clickCount(0), m_lastClickPos(0, 0) {
    setObjectName("CustomTitleBar");
    setFixedHeight(26);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(6, 0, 0, 0);
    m_layout->setSpacing(1);

    // App icon on the left - используем QPushButton для лучшей обработки кликов
    m_appIconButton = new QPushButton(this);
    {
        QPixmap pm(":/icons/icons/logo/vuzhyk.ico");
        if (!pm.isNull()) {
            m_appIconButton->setIcon(QIcon(pm));
            m_appIconButton->setIconSize(QSize(20, 20));
        }
        m_appIconButton->setFixedSize(22, 22);
        m_appIconButton->setFlat(true);
        m_appIconButton->setStyleSheet("QPushButton { border: none; background: transparent; }"
                                       "QPushButton:hover { background: transparent; }"
                                       "QPushButton:pressed { background: transparent; }"
                                       "QPushButton:focus { outline: none; }");
        connect(m_appIconButton, &QPushButton::clicked, this, [this]() {
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
            QPoint currentPos = QCursor::pos();
            
            // Проверяем, был ли это восьмерной клик (в пределах 500 мс и примерно в том же месте)
            if (currentTime - m_lastClickTime < 500 && 
                (currentPos - m_lastClickPos).manhattanLength() < 20) {
                m_clickCount++;
                if (m_clickCount >= 8) {
                    emit iconTripleClicked();
                    m_clickCount = 0;
                    m_lastClickTime = 0;
                }
            } else {
                m_clickCount = 1;
            }
            
            m_lastClickTime = currentTime;
            m_lastClickPos = currentPos;
        });
    }
    m_layout->addWidget(m_appIconButton, 0);
    
    // m_appIconLabel остается nullptr, так как теперь используем кнопку

    if (m_menuBar) {
        m_menuBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_menuBar->setContentsMargins(0,0,0,0);
        // Increase font size by 20%
        QFont f = m_menuBar->font();
        if (f.pointSizeF() > 0) {
            f.setPointSizeF(f.pointSizeF() * 1.2);
        } else if (f.pixelSize() > 0) {
            f.setPixelSize(int(f.pixelSize() * 1.2));
        }
        m_menuBar->setFont(f);
        const int h = height();
        m_menuBar->setMinimumHeight(h);
        m_menuBar->setMaximumHeight(h);
        m_menuBar->setFixedHeight(h);
        m_menuBar->setStyleSheet(QString(
            "QMenuBar{background:transparent;border:none;padding:0;margin:0;min-height:%1px;}"
            "QMenuBar::item{padding:0 12px;margin:0;border:0;min-height:%1px;}"
            "QMenuBar::item:selected{background:rgba(0,0,0,0.06);}"
        ).arg(h));
        m_layout->addWidget(m_menuBar, 1);
    }

    const QSize iconSz(21, 21);
    const QSize btnSz(36, 28);
    const char *btnBaseCss = "QToolButton{background:transparent;border:none;padding:0;margin:0;outline:0;}"
                             "QToolButton:hover{background:transparent;}"
                             "QToolButton:pressed{background:transparent;}";
    // Prepare icons (will be updated based on theme)
    m_minIcon         = new QIcon();
    m_minIconHover    = new QIcon();
    m_maxIcon         = new QIcon();
    m_maxIconHover    = new QIcon();
    m_restoreIcon     = new QIcon();
    m_restoreIconHover= new QIcon();
    m_closeIcon       = new QIcon();
    m_closeIconHover  = new QIcon();
    m_settingsIcon    = new QIcon();
    m_settingsIconHover = new QIcon();
    
    // Initialize with default theme (light)
    updateIconsForTheme("light");

    // Settings gear icon (left of window controls)
    m_settingsButton = new QToolButton(this);
    m_settingsButton->setIcon(*m_settingsIcon);
    m_settingsButton->setIconSize(QSize(16, 16)); // 20% smaller (20 * 0.8 = 16)
    m_settingsButton->setFixedSize(QSize(18, height())); // 20% smaller width
    m_settingsButton->setAutoRaise(true);
    m_settingsButton->setStyleSheet(btnBaseCss);
    connect(m_settingsButton, &QToolButton::clicked, this, &TitleBar::settingsClicked);
    m_settingsButton->installEventFilter(this);
    m_layout->addWidget(m_settingsButton, 0);

    m_minButton = new QToolButton(this);
    m_minButton->setIcon(*m_minIcon);
    m_minButton->setIconSize(iconSz);
    m_minButton->setFixedSize(QSize(btnSz.width(), height()));
    m_minButton->setAutoRaise(true);
    m_minButton->setStyleSheet(btnBaseCss);
    m_minButton->setToolTip(tr("Свернуть"));
    connect(m_minButton, &QToolButton::clicked, this, &TitleBar::onMinimize);
    m_minButton->installEventFilter(this);

    m_maxButton = new QToolButton(this);
    m_maxButton->setToolTip(tr("Развернуть/Восстановить"));
    m_maxButton->setIconSize(iconSz);
    m_maxButton->setFixedSize(QSize(btnSz.width(), height()));
    m_maxButton->setAutoRaise(true);
    m_maxButton->setStyleSheet(btnBaseCss);
    connect(m_maxButton, &QToolButton::clicked, this, &TitleBar::onMaximizeRestore);
    updateMaximizeIcon();
    m_maxButton->installEventFilter(this);

    m_closeButton = new QToolButton(this);
    m_closeButton->setIcon(*m_closeIcon);
    m_closeButton->setIconSize(QSize(25, 25));
    m_closeButton->setFixedSize(QSize(43, height()));
    m_closeButton->setAutoRaise(true);
    m_closeButton->setStyleSheet(btnBaseCss);
    m_closeButton->setToolTip(tr("Закрыть"));
    connect(m_closeButton, &QToolButton::clicked, this, &TitleBar::onClose);
    m_closeButton->installEventFilter(this);

    m_layout->addWidget(m_minButton, 0);
    m_layout->addWidget(m_maxButton, 0);
    m_layout->addWidget(m_closeButton, 0);
    setLayout(m_layout);
}

void TitleBar::mousePressEvent(QMouseEvent *event) {
    if (!m_window) return;
    
    // Проверяем, был ли клик по иконке (кнопке)
    if (m_appIconButton && event->button() == Qt::LeftButton) {
        QPoint localPos = m_appIconButton->mapFromGlobal(event->globalPos());
        QRect iconRect = m_appIconButton->rect();
        if (iconRect.contains(localPos)) {
            // Обработка клика теперь в сигнале clicked кнопки
            event->accept();
            return; // Не обрабатываем как перетаскивание
        }
    }
    
    if (event->button() == Qt::LeftButton) {
        m_dragOffset = event->globalPos() - m_window->frameGeometry().topLeft();
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event) {
    if (!m_window) return;
    if (event->buttons() & Qt::LeftButton) {
        if (!m_window->isMaximized()) {
            m_window->move(event->globalPos() - m_dragOffset);
            event->accept();
            return;
        }
    }
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        onMaximizeRestore();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void TitleBar::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (m_menuBar) {
        const int h = height();
        m_menuBar->setMinimumHeight(h);
        m_menuBar->setMaximumHeight(h);
        m_menuBar->setFixedHeight(h);
        m_menuBar->setStyleSheet(QString(
            "QMenuBar{background:transparent;border:none;padding:0;margin:0;min-height:%1px;}"
            "QMenuBar::item{padding:0 12px;margin:0;border:0;min-height:%1px;}"
            "QMenuBar::item:selected{background:rgba(0,0,0,0.06);}"
        ).arg(h));
        if (m_minButton) m_minButton->setFixedHeight(h);
        if (m_maxButton) m_maxButton->setFixedHeight(h);
        if (m_closeButton) m_closeButton->setFixedHeight(h);
    }
}

void TitleBar::onMinimize() {
    if (m_window) m_window->showMinimized();
}

void TitleBar::onMaximizeRestore() {
    if (!m_window) return;
    if (m_window->isMaximized()) m_window->showNormal(); else m_window->showMaximized();
    updateMaximizeIcon();
}

void TitleBar::onClose() {
    if (m_window) m_window->close();
}

void TitleBar::updateMaximizeIcon() {
    if (!m_window) return;
    const bool maximized = m_window->isMaximized();
    if (maximized) {
        m_maxButton->setIcon(*m_restoreIcon);
    } else {
        m_maxButton->setIcon(*m_maxIcon);
    }
}

bool TitleBar::eventFilter(QObject *watched, QEvent *event) {
    // Обработка кликов по иконке теперь через сигнал clicked кнопки
    // eventFilter оставляем для других виджетов (кнопки управления окном)
    
    if (watched == m_minButton) {
        if (event->type() == QEvent::Enter) {
            m_minButton->setIcon(*m_minIconHover);
        } else if (event->type() == QEvent::Leave) {
            m_minButton->setIcon(*m_minIcon);
        }
    } else if (watched == m_maxButton) {
        if (event->type() == QEvent::Enter) {
            if (m_window && m_window->isMaximized()) m_maxButton->setIcon(*m_restoreIconHover);
            else m_maxButton->setIcon(*m_maxIconHover);
        } else if (event->type() == QEvent::Leave) {
            if (m_window && m_window->isMaximized()) m_maxButton->setIcon(*m_restoreIcon);
            else m_maxButton->setIcon(*m_maxIcon);
        }
    } else if (watched == m_closeButton) {
        if (event->type() == QEvent::Enter) {
            m_closeButton->setIcon(*m_closeIconHover);
        } else if (event->type() == QEvent::Leave) {
            m_closeButton->setIcon(*m_closeIcon);
        }
    } else if (watched == m_settingsButton) {
        if (event->type() == QEvent::Enter) {
            m_settingsButton->setIcon(*m_settingsIconHover);
        } else if (event->type() == QEvent::Leave) {
            m_settingsButton->setIcon(*m_settingsIcon);
        }
    }
    return QWidget::eventFilter(watched, event);
}

QIcon TitleBar::createThemedIcon(const QString &resourcePath, const QString &color) {
    QIcon icon;
    
    // Загружаем SVG из ресурсов
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QIcon(resourcePath); // Fallback
    }
    QByteArray svgData = file.readAll();
    file.close();
    
    // Если цвет не черный, перекрашиваем SVG
    if (color != "#000000" && color != "black") {
        QString svgString = QString::fromUtf8(svgData);
        // Заменяем черный цвет на новый цвет
        svgString.replace("#000000", color, Qt::CaseInsensitive);
        svgString.replace("black", color, Qt::CaseInsensitive);
        svgString.replace("rgb(0,0,0)", color, Qt::CaseInsensitive);
        svgData = svgString.toUtf8();
    }
    
    // Создаем иконку из SVG через QSvgRenderer
    QSvgRenderer renderer(svgData);
    if (renderer.isValid()) {
        // Создаем несколько размеров для лучшего качества
        for (int size : {16, 21, 25, 32}) {
            QPixmap pixmap(size, size);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            renderer.render(&painter);
            painter.end();
            icon.addPixmap(pixmap);
        }
    } else {
        icon = QIcon(resourcePath);
    }
    
    return icon;
}

void TitleBar::updateIconsForTheme(const QString &theme) {
    QString iconColor = (theme == "dark") ? "#ffffff" : "#000000";
    QString basePath = ":/icons/icons/window-controls/";
    
    // Обновляем иконки
    *m_minIcon = createThemedIcon(basePath + "window-minimize.svg", iconColor);
    *m_maxIcon = createThemedIcon(basePath + "window-maximize.svg", iconColor);
    *m_restoreIcon = createThemedIcon(basePath + "window-restore.svg", iconColor);
    *m_closeIcon = createThemedIcon(basePath + "window-close.svg", iconColor);
    *m_settingsIcon = createThemedIcon(basePath + "settings-gear.svg", iconColor);
    
    // Для hover иконок используем оригинальные цвета (синий для min/max/restore, красный для close)
    *m_minIconHover = createThemedIcon(basePath + "window-minimize-hover.svg", (theme == "dark") ? "#ffffff" : "#0080ff");
    *m_maxIconHover = createThemedIcon(basePath + "window-maximize-hover.svg", (theme == "dark") ? "#ffffff" : "#0080ff");
    *m_restoreIconHover = createThemedIcon(basePath + "window-restore-hover.svg", (theme == "dark") ? "#ffffff" : "#0080ff");
    *m_closeIconHover = createThemedIcon(basePath + "window-close-hover.svg", (theme == "dark") ? "#ffffff" : "#ff0000");
    *m_settingsIconHover = createThemedIcon(basePath + "settings-gear-hover.svg", (theme == "dark") ? "#ffffff" : "#0080ff");
    
    // Обновляем иконки на кнопках
    if (m_minButton) {
        m_minButton->setIcon(*m_minIcon);
    }
    if (m_maxButton) {
        updateMaximizeIcon();
    }
    if (m_closeButton) {
        m_closeButton->setIcon(*m_closeIcon);
    }
    // Обновляем иконку шестеренки
    if (m_settingsButton) {
        m_settingsButton->setIcon(*m_settingsIcon);
    }
}



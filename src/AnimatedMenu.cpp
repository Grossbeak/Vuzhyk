#include "AnimatedMenu.h"

#include <QAbstractAnimation>
#include <QEasingCurve>
#include <QEvent>
#include <QRegion>
#include <QShowEvent>

#include <algorithm>

namespace {
constexpr int kAnimationDurationMs = 160;
}

AnimatedMenu::AnimatedMenu(QWidget *parent)
    : QMenu(parent) {
    initializeAnimation();
}

AnimatedMenu::AnimatedMenu(const QString &title, QWidget *parent)
    : QMenu(title, parent) {
    initializeAnimation();
}

void AnimatedMenu::initializeAnimation() {
    if (!m_animation) {
        m_animation = new QVariantAnimation(this);
    }
    m_animation->setDuration(kAnimationDurationMs);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        const int h = std::max(0, value.toInt());
        const int w = width();
        if (h <= 0 || w <= 0) {
            setMask(QRegion());
            return;
        }
        setMask(QRegion(0, 0, w, h));
    });

    connect(m_animation, &QVariantAnimation::finished, this, [this]() {
        if (m_closing) {
            clearMask();
            m_blockClose = true;
            QMenu::hide();
            m_blockClose = false;
            m_closing = false;
        } else {
            clearMask();
        }
    });
}

void AnimatedMenu::showEvent(QShowEvent *event) {
    QMenu::showEvent(event);
    startOpenAnimation();
}

bool AnimatedMenu::event(QEvent *event) {
    if (event->type() == QEvent::Hide && !m_blockClose && isVisible()) {
        event->ignore();
        startCloseAnimation();
        return true;
    }
    return QMenu::event(event);
}

void AnimatedMenu::startOpenAnimation() {
    if (!isVisible()) {
        return;
    }

    m_closing = false;
    if (m_animation->state() == QAbstractAnimation::Running) {
        m_animation->stop();
    }

    const int targetHeight = std::max(height(), sizeHint().height());
    const int w = width();
    if (targetHeight <= 0 || w <= 0) {
        return;
    }

    setMask(QRegion(0, 0, w, 0));
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    m_animation->setStartValue(0);
    m_animation->setEndValue(targetHeight);
    m_animation->start();
}

void AnimatedMenu::startCloseAnimation() {
    if (!isVisible()) {
        return;
    }
    m_closing = true;

    if (m_animation->state() == QAbstractAnimation::Running) {
        m_animation->stop();
    }

    const int startHeight = std::max(height(), sizeHint().height());
    const int w = width();
    if (startHeight <= 0 || w <= 0) {
        m_blockClose = true;
        QMenu::hide();
        m_blockClose = false;
        m_closing = false;
        return;
    }

    setMask(QRegion(0, 0, w, startHeight));
    m_animation->setEasingCurve(QEasingCurve::InCubic);
    m_animation->setStartValue(startHeight);
    m_animation->setEndValue(0);
    m_animation->start();
}

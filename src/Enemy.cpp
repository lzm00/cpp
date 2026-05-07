#include "Enemy.h"

#include <QLineF>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QtMath>

namespace
{
QPixmap enemyIcon(EnemyType type)
{
    switch (type) {
    case EnemyType::Assassin:
        return QPixmap(QStringLiteral("D:/cpp/picture/8ef82fc0cb7123709b104be3c35f44b3.jpg"));
    case EnemyType::Tank:
        return QPixmap(QStringLiteral("D:/cpp/picture/a84d2ae14aa405704cec3430c6689715.png"));
    case EnemyType::Grunt:
        return QPixmap(QStringLiteral("D:/cpp/picture/a1036be708507b658f671266f2497722.png"));
    }
    return QPixmap();
}
}

Enemy::Enemy(EnemyType type, const QVector<QPointF> &path, int waveIndex)
    : m_path(path),
      m_type(type)
{
    switch (type) {
    case EnemyType::Grunt:
        m_maxHp = 90;
        m_speed = 72.0;
        m_reward = 65;
        m_size = 28.0;
        break;
    case EnemyType::Tank:
        m_maxHp = 190;
        m_speed = 46.0;
        m_reward = 150;
        m_size = 42.0;
        m_baseDamage = 2;
        break;
    case EnemyType::Assassin:
        m_maxHp = 62;
        m_speed = 108.0;
        m_reward = 95;
        m_size = 26.0;
        break;
    }

    m_maxHp = qRound(m_maxHp * (1.0 + waveIndex * 0.18));
    m_speed *= 1.0 + waveIndex * 0.035;
    m_reward += waveIndex * 5;
    m_hp = m_maxHp;
    if (!m_path.isEmpty()) {
        setPos(m_path.first());
    }
    setZValue(10);
}

QRectF Enemy::boundingRect() const
{
    const qreal visualSize = qMax<qreal>(m_size, 36.0);
    const qreal half = visualSize / 2.0;
    return QRectF(-half - 4, -half - 12, visualSize + 8, visualSize + 20);
}

void Enemy::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::Antialiasing, true);

    const qreal half = m_size / 2.0;
    painter->setPen(QPen(QColor(70, 45, 36), 2));

    const QPixmap icon = enemyIcon(m_type);
    if (!icon.isNull()) {
        const qreal iconSize = qMax<qreal>(m_size, 36.0);
        const qreal iconHalf = iconSize / 2.0;
        painter->setBrush(QColor(45, 36, 32, 170));
        painter->drawEllipse(QRectF(-iconHalf, -iconHalf, iconSize, iconSize));
        painter->drawPixmap(QRectF(-iconHalf, -iconHalf, iconSize, iconSize),
                            icon,
                            QRectF(QPointF(0, 0), QSizeF(icon.size())));
    } else if (m_type == EnemyType::Grunt || m_type == EnemyType::Assassin) {
        painter->setBrush(m_type == EnemyType::Assassin ? QColor(197, 66, 92) : QColor(218, 84, 50));
        painter->drawEllipse(QRectF(-half, -half, m_size, m_size));

        painter->setBrush(m_type == EnemyType::Assassin ? QColor(255, 205, 117) : QColor(245, 188, 96));
        painter->drawPolygon(QPolygonF({
            QPointF(-half * 0.45, -half * 0.35),
            QPointF(0, -half * 0.95),
            QPointF(half * 0.45, -half * 0.35)
        }));

        painter->setPen(QPen(QColor(255, 225, 155), 2));
        painter->drawLine(QPointF(-half * 0.9, half * 0.1), QPointF(-half * 1.35, half * 0.55));
        painter->drawLine(QPointF(half * 0.9, half * 0.1), QPointF(half * 1.35, half * 0.55));
        if (m_type == EnemyType::Assassin) {
            painter->setPen(QPen(QColor(255, 238, 184), 2));
            painter->drawLine(QPointF(-half * 0.75, -half * 0.75), QPointF(half * 0.75, half * 0.75));
            painter->drawLine(QPointF(half * 0.75, -half * 0.75), QPointF(-half * 0.75, half * 0.75));
        }
    } else {
        painter->setBrush(QColor(178, 68, 45));
        painter->drawRoundedRect(QRectF(-half, -half * 0.6, m_size, m_size * 0.9), 7, 7);

        painter->setBrush(QColor(105, 76, 65));
        painter->drawEllipse(QRectF(-half * 0.85, half * 0.12, half * 0.7, half * 0.7));
        painter->drawEllipse(QRectF(half * 0.15, half * 0.12, half * 0.7, half * 0.7));

        painter->setBrush(QColor(218, 175, 90));
        painter->drawRoundedRect(QRectF(-half * 0.2, -half * 0.9, half * 1.25, half * 0.38), 4, 4);
        painter->setPen(QPen(QColor(70, 45, 36), 3));
        painter->drawLine(QPointF(half * 0.3, -half * 0.72), QPointF(half * 1.35, -half * 0.72));
    }

    const QRectF backBar(-half - 3, -half - 10, m_size + 6, 5);
    painter->setBrush(QColor(60, 57, 53));
    painter->drawRoundedRect(backBar, 2, 2);

    QRectF hpBar = backBar.adjusted(1, 1, -1, -1);
    hpBar.setWidth(hpBar.width() * hpRatio());
    painter->setBrush(QColor(103, 186, 119));
    painter->drawRoundedRect(hpBar, 1.5, 1.5);
}

void Enemy::updateObject(qreal dt)
{
    if (m_reachedEnd || isDead() || m_path.size() < 2) {
        return;
    }

    if (m_slowTime > 0.0) {
        m_slowTime -= dt;
        if (m_slowTime <= 0.0) {
            m_slowMultiplier = 1.0;
            m_slowTime = 0.0;
        }
    }

    if (isFrozen) {
        return;
    }

    QPointF current = pos();
    QPointF target = m_path.value(m_waypointIndex);
    QLineF toTarget(current, target);
    const qreal speedMultiplier = isSpeedBoosted ? m_speedBoostMultiplier : 1.0;
    const qreal step = m_speed * m_slowMultiplier * speedMultiplier * dt;

    if (toTarget.length() <= step) {
        setPos(target);
        ++m_waypointIndex;
        if (m_waypointIndex >= m_path.size()) {
            m_reachedEnd = true;
        }
        return;
    }

    const qreal angle = qDegreesToRadians(-toTarget.angle());
    setPos(current + QPointF(qCos(angle) * step, qSin(angle) * step));
}

void Enemy::applyDamage(int amount)
{
    if (isShielded) {
        return;
    }

    m_hp -= amount;
    update();
}

void Enemy::applySlow(qreal multiplier, qreal duration)
{
    m_slowMultiplier = qMin(m_slowMultiplier, multiplier);
    m_slowTime = qMax(m_slowTime, duration);
}

void Enemy::freezeFor(int durationMs)
{
    isFrozen = true;
    const int token = ++m_freezeToken;
    update();

    QTimer::singleShot(durationMs, this, [this, token]() {
        if (token != m_freezeToken) {
            return;
        }
        isFrozen = false;
        update();
    });
}

void Enemy::shieldFor(int durationMs)
{
    isShielded = true;
    const int token = ++m_shieldToken;
    update();

    QTimer::singleShot(durationMs, this, [this, token]() {
        if (token != m_shieldToken) {
            return;
        }
        isShielded = false;
        update();
    });
}

void Enemy::speedBoostFor(int durationMs, qreal multiplier)
{
    isSpeedBoosted = true;
    m_speedBoostMultiplier = multiplier;
    const int token = ++m_speedBoostToken;
    update();

    QTimer::singleShot(durationMs, this, [this, token]() {
        if (token != m_speedBoostToken) {
            return;
        }
        isSpeedBoosted = false;
        m_speedBoostMultiplier = 1.0;
        update();
    });
}

void Enemy::setPath(const QVector<QPointF> &path)
{
    m_path = path;
    m_waypointIndex = 1;
    m_reachedEnd = false;
    if (!m_path.isEmpty()) {
        setPos(m_path.first());
    }
}

bool Enemy::isDead() const
{
    return m_hp <= 0;
}

bool Enemy::reachedEnd() const
{
    return m_reachedEnd;
}

int Enemy::reward() const
{
    return m_reward;
}

int Enemy::baseDamage() const
{
    return m_baseDamage;
}

qreal Enemy::hpRatio() const
{
    return qBound(0.0, static_cast<qreal>(m_hp) / static_cast<qreal>(m_maxHp), 1.0);
}

qreal Enemy::pathProgress() const
{
    return static_cast<qreal>(m_waypointIndex) / qMax(1, m_path.size());
}

EnemyType Enemy::enemyType() const
{
    return m_type;
}

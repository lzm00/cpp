#include "Enemy.h"

#include <QLineF>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QtMath>

namespace
{
// 根据敌人类型选择图片素材；找不到图片时，paint() 会画备用图形。
QPixmap enemyIcon(EnemyType type)
{
    switch (type) {
    case EnemyType::Assassin:
        return QPixmap(QStringLiteral("D:/cpp/picture/8ef82fc0cb7123709b104be3c35f44b3.jpg"));
    case EnemyType::Tank:
        return QPixmap(QStringLiteral("D:/cpp/picture/a84d2ae14aa405704cec3430c6689715.png"));
    case EnemyType::Grunt:
        return QPixmap(QStringLiteral("D:/cpp/picture/a1036be708507b658f671266f2497722.png"));
    case EnemyType::Priest:
    case EnemyType::Boss:
        return QPixmap();
    }
    return QPixmap();
}
}

Enemy::Enemy(EnemyType type, const QVector<QPointF> &path, int waveIndex)
    : m_path(path),
      m_type(type)
{
    // 不同敌人类型先设置基础数值。
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
    case EnemyType::Priest:
        m_maxHp = 130;
        m_speed = 52.0;
        m_reward = 125;
        m_size = 32.0;
        m_specialCooldown = 2.2;
        break;
    case EnemyType::Boss:
        m_maxHp = 950;
        m_speed = 34.0;
        m_reward = 700;
        m_size = 68.0;
        m_baseDamage = 5;
        m_specialCooldown = 4.0;
        if (waveIndex >= 10) {
            m_maxHp = 1450;
            m_reward = 1000;
            m_baseDamage = 6;
        }
        if (waveIndex >= 15) {
            m_maxHp = 2100;
            m_reward = 1350;
            m_baseDamage = 7;
        }
        if (waveIndex >= 20) {
            m_maxHp = 3600;
            m_reward = 2500;
            m_baseDamage = 10;
            m_size = 82.0;
        }
        break;
    }

    // 波次越靠后，敌人的生命、速度和奖励都会提高。
    m_maxHp = qRound(m_maxHp * (1.0 + waveIndex * 0.15));
    m_speed *= 1.0 + waveIndex * 0.02;
    m_reward += waveIndex * 5;
    if (m_type == EnemyType::Boss) {
        m_speed = qMin<qreal>(m_speed, 44.0);
        m_reward += waveIndex * 25;
    }
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
    if (isHidden()) {
        painter->setOpacity(0.35);
    }

    const qreal half = m_size / 2.0;
    painter->setPen(QPen(QColor(70, 45, 36), 2));

    const QPixmap icon = enemyIcon(m_type);
    if (m_type == EnemyType::Priest) {
        painter->setBrush(QColor(87, 107, 170));
        painter->drawEllipse(QRectF(-half, -half, m_size, m_size));
        painter->setBrush(QColor(233, 217, 136));
        painter->drawEllipse(QRectF(-half * 0.50, -half * 0.92, half, half));
        painter->setPen(QPen(QColor(235, 245, 255), 3));
        painter->drawLine(QPointF(0, -half * 0.2), QPointF(0, half * 0.75));
        painter->drawLine(QPointF(-half * 0.35, half * 0.25), QPointF(half * 0.35, half * 0.25));
        painter->setPen(QPen(QColor(132, 205, 170), 2));
        painter->drawArc(QRectF(-half * 0.9, -half * 0.9, half * 1.8, half * 1.8), 30 * 16, 120 * 16);
    } else if (m_type == EnemyType::Boss) {
        painter->setBrush(QColor(82, 42, 104));
        painter->drawRoundedRect(QRectF(-half, -half * 0.70, m_size, m_size * 1.05), 12, 12);
        painter->setBrush(QColor(205, 71, 66));
        painter->drawEllipse(QRectF(-half * 0.58, -half * 1.05, half * 1.16, half * 0.92));
        painter->setBrush(QColor(246, 199, 78));
        painter->drawPolygon(QPolygonF({
            QPointF(-half * 0.65, -half * 1.05),
            QPointF(-half * 0.35, -half * 1.45),
            QPointF(-half * 0.10, -half * 1.03)
        }));
        painter->drawPolygon(QPolygonF({
            QPointF(half * 0.65, -half * 1.05),
            QPointF(half * 0.35, -half * 1.45),
            QPointF(half * 0.10, -half * 1.03)
        }));
        painter->setPen(QPen(QColor(255, 230, 120), 4));
        painter->drawLine(QPointF(-half * 0.85, half * 0.25), QPointF(half * 0.85, half * 0.25));
    } else if (!icon.isNull()) {
        // 优先使用外部图片素材绘制敌人。
        const qreal iconSize = qMax<qreal>(m_size, 36.0);
        const qreal iconHalf = iconSize / 2.0;
        painter->setBrush(QColor(45, 36, 32, 170));
        painter->drawEllipse(QRectF(-iconHalf, -iconHalf, iconSize, iconSize));
        painter->drawPixmap(QRectF(-iconHalf, -iconHalf, iconSize, iconSize),
                            icon,
                            QRectF(QPointF(0, 0), QSizeF(icon.size())));
    } else if (m_type == EnemyType::Grunt || m_type == EnemyType::Assassin) {
        // 图片缺失时，用 QPainter 画一个简单的普通/快速敌人形状。
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
        // 图片缺失时，用 QPainter 画一个简单的坦克敌人形状。
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

    // 敌人头顶血条。
    const QRectF backBar(-half - 3, -half - 10, m_size + 6, 5);
    painter->setBrush(QColor(60, 57, 53));
    painter->drawRoundedRect(backBar, 2, 2);

    QRectF hpBar = backBar.adjusted(1, 1, -1, -1);
    hpBar.setWidth(hpBar.width() * hpRatio());
    painter->setBrush(QColor(103, 186, 119));
    painter->drawRoundedRect(hpBar, 1.5, 1.5);

    if (isShielded) {
        painter->setPen(QPen(QColor(126, 204, 255), 3));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QRectF(-half - 5, -half - 5, m_size + 10, m_size + 10));
    }
}

void Enemy::updateObject(qreal dt)
{
    if (m_reachedEnd || isDead() || m_path.size() < 2) {
        return;
    }

    // 减速效果按时间衰减，结束后恢复正常速度倍率。
    if (m_slowTime > 0.0) {
        m_slowTime -= dt;
        if (m_slowTime <= 0.0) {
            m_slowMultiplier = 1.0;
            m_slowTime = 0.0;
        }
    }

    if (m_vulnerabilityTime > 0.0) {
        m_vulnerabilityTime -= dt;
        if (m_vulnerabilityTime <= 0.0) {
            m_vulnerabilityMultiplier = 1.0;
            m_vulnerabilityTime = 0.0;
        }
    }

    if (m_specialCooldown > 0.0) {
        m_specialCooldown = qMax<qreal>(0.0, m_specialCooldown - dt);
    }

    if (m_hiddenTime > 0.0) {
        m_hiddenTime = qMax<qreal>(0.0, m_hiddenTime - dt);
    }

    if (isFrozen) {
        return;
    }

    // 朝当前路径点移动；到达后切换到下一个路径点。
    QPointF current = pos();
    QPointF target = m_path.value(m_waypointIndex);
    QLineF toTarget(current, target);
    const qreal speedMultiplier = isSpeedBoosted ? m_speedBoostMultiplier : 1.0;
    const qreal step = m_speed * m_slowMultiplier * m_terrainSpeedMultiplier * speedMultiplier * dt;

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
    // 护盾状态下不受伤害。
    if (isShielded) {
        return;
    }

    m_hp -= qRound(amount * m_vulnerabilityMultiplier);
    update();
}

void Enemy::heal(int amount)
{
    if (amount <= 0 || isDead()) {
        return;
    }

    m_hp = qMin(m_maxHp, m_hp + amount);
    update();
}

void Enemy::applySlow(qreal multiplier, qreal duration)
{
    m_slowMultiplier = qMin(m_slowMultiplier, multiplier);
    m_slowTime = qMax(m_slowTime, duration);
}

void Enemy::applyBurn(int damagePerTick, int ticks, int intervalMs)
{
    if (damagePerTick <= 0 || ticks <= 0 || intervalMs <= 0) {
        return;
    }

    for (int i = 1; i <= ticks; ++i) {
        QTimer::singleShot(intervalMs * i, this, [this, damagePerTick]() {
            if (!m_reachedEnd && !isDead()) {
                applyDamage(damagePerTick);
            }
        });
    }
}

void Enemy::applyVulnerability(qreal multiplier, qreal duration)
{
    m_vulnerabilityMultiplier = qMax(m_vulnerabilityMultiplier, multiplier);
    m_vulnerabilityTime = qMax(m_vulnerabilityTime, duration);
}

void Enemy::freezeFor(int durationMs)
{
    isFrozen = true;
    const int token = ++m_freezeToken;
    update();

    // singleShot 到期后解除冻结；token 防止旧定时器误清除新冻结。
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

void Enemy::updateTerrainState(bool inRiver, bool inBush)
{
    m_terrainSpeedMultiplier = inRiver ? 0.58 : 1.0;
    if (inBush) {
        m_hiddenTime = qMax(m_hiddenTime, 0.75);
    }
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

void Enemy::setWaypointIndex(int index)
{
    m_waypointIndex = qBound(1, index, qMax(1, m_path.size() - 1));
}

bool Enemy::isDead() const
{
    return m_hp <= 0;
}

bool Enemy::reachedEnd() const
{
    return m_reachedEnd;
}

bool Enemy::isHidden() const
{
    return m_hiddenTime > 0.0;
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
    // 炮塔用这个值判断哪个敌人更靠近终点。
    return static_cast<qreal>(m_waypointIndex) / qMax(1, m_path.size());
}

EnemyType Enemy::enemyType() const
{
    return m_type;
}

int Enemy::waypointIndex() const
{
    return m_waypointIndex;
}

bool Enemy::specialReady() const
{
    return m_specialCooldown <= 0.0;
}

void Enemy::resetSpecialCooldown(qreal seconds)
{
    m_specialCooldown = seconds;
}

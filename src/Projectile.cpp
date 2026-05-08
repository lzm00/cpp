#include "Projectile.h"

#include "Enemy.h"
#include "GameScene.h"

#include <QLineF>
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>

Projectile::Projectile(const QPointF &start,
                       Enemy *target,
                       int damage,
                       qreal slowMultiplier,
                       qreal slowDuration,
                       qreal critChance,
                       qreal critMultiplier,
                       qreal explosionRadius,
                       qreal explosionDamageRatio,
                       int burnDamage,
                       int burnTicks,
                       int burnIntervalMs,
                       const QColor &color)
    : m_target(target),
      m_damage(damage),
      m_slowMultiplier(slowMultiplier),
      m_slowDuration(slowDuration),
      m_critChance(critChance),
      m_critMultiplier(critMultiplier),
      m_explosionRadius(explosionRadius),
      m_explosionDamageRatio(explosionDamageRatio),
      m_burnDamage(burnDamage),
      m_burnTicks(burnTicks),
      m_burnIntervalMs(burnIntervalMs),
      m_color(color)
{
    // 子弹从炮塔位置生成，显示层级高于敌人和炮塔。
    setPos(start);
    setZValue(30);
}

QRectF Projectile::boundingRect() const
{
    return QRectF(-6, -6, 12, 12);
}

void Projectile::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(QColor(54, 48, 42), 1.4));
    painter->setBrush(m_color);
    painter->drawEllipse(QRectF(-5, -5, 10, 10));
}

void Projectile::updateObject(qreal dt)
{
    if (!m_target || m_target->isDead()) {
        m_finished = true;
        return;
    }

    // 每帧朝目标当前位置移动，所以敌人移动时子弹也会跟踪。
    const QPointF current = pos();
    const QPointF targetPos = m_target->pos();
    QLineF toTarget(current, targetPos);
    const qreal step = m_speed * dt;

    if (toTarget.length() <= step + 8.0) {
        // 进入命中距离后结算伤害；辅助塔子弹还会附加减速。
        int finalDamage = m_damage;
        if (m_critChance > 0.0 && QRandomGenerator::global()->generateDouble() < m_critChance) {
            finalDamage = qRound(finalDamage * m_critMultiplier);
        }

        m_target->applyDamage(finalDamage);
        if (m_slowMultiplier < 1.0) {
            m_target->applySlow(m_slowMultiplier, m_slowDuration);
        }
        if (m_burnDamage > 0 && m_burnTicks > 0) {
            m_target->applyBurn(m_burnDamage, m_burnTicks, m_burnIntervalMs);
        }
        if (m_explosionRadius > 0.0 && m_explosionDamageRatio > 0.0) {
            if (auto *gameScene = dynamic_cast<GameScene *>(scene())) {
                const int explosionDamage = qMax(1, qRound(finalDamage * m_explosionDamageRatio));
                const auto enemies = gameScene->enemiesInRange(m_target->pos(), m_explosionRadius);
                for (Enemy *enemy : enemies) {
                    if (enemy && enemy != m_target && !enemy->isDead()) {
                        enemy->applyDamage(explosionDamage);
                    }
                }
            }
        }
        m_finished = true;
        return;
    }

    const qreal angle = qDegreesToRadians(-toTarget.angle());
    setPos(current + QPointF(qCos(angle) * step, qSin(angle) * step));
}

bool Projectile::isFinished() const
{
    return m_finished;
}

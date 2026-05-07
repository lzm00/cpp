#include "Projectile.h"

#include "Enemy.h"

#include <QLineF>
#include <QPainter>
#include <QtMath>

Projectile::Projectile(const QPointF &start,
                       Enemy *target,
                       int damage,
                       qreal slowMultiplier,
                       qreal slowDuration,
                       const QColor &color)
    : m_target(target),
      m_damage(damage),
      m_slowMultiplier(slowMultiplier),
      m_slowDuration(slowDuration),
      m_color(color)
{
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

    const QPointF current = pos();
    const QPointF targetPos = m_target->pos();
    QLineF toTarget(current, targetPos);
    const qreal step = m_speed * dt;

    if (toTarget.length() <= step + 8.0) {
        m_target->applyDamage(m_damage);
        if (m_slowMultiplier < 1.0) {
            m_target->applySlow(m_slowMultiplier, m_slowDuration);
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

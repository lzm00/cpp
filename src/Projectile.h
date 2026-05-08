#pragma once

#include "GameObject.h"

#include <QColor>
#include <QGraphicsItem>
#include <QPointer>

class Enemy;

// Projectile 表示炮塔发出的子弹：追踪一个目标，命中后造成伤害和可能的减速。
class Projectile : public QGraphicsItem, public GameObject
{
public:
    Projectile(const QPointF &start,
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
               const QColor &color);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void updateObject(qreal dt) override;

    bool isFinished() const;

private:
    // QPointer 会在目标对象被删除后自动变为空，避免悬空指针。
    QPointer<Enemy> m_target;
    int m_damage = 0;
    qreal m_speed = 360.0;
    qreal m_slowMultiplier = 1.0;
    qreal m_slowDuration = 0.0;
    qreal m_critChance = 0.0;
    qreal m_critMultiplier = 2.0;
    qreal m_explosionRadius = 0.0;
    qreal m_explosionDamageRatio = 0.0;
    int m_burnDamage = 0;
    int m_burnTicks = 0;
    int m_burnIntervalMs = 450;
    QColor m_color;
    bool m_finished = false;
};

#pragma once

#include "GameObject.h"

#include <QColor>
#include <QGraphicsItem>
#include <QPointer>

class Enemy;

class Projectile : public QGraphicsItem, public GameObject
{
public:
    Projectile(const QPointF &start,
               Enemy *target,
               int damage,
               qreal slowMultiplier,
               qreal slowDuration,
               const QColor &color);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void updateObject(qreal dt) override;

    bool isFinished() const;

private:
    QPointer<Enemy> m_target;
    int m_damage = 0;
    qreal m_speed = 360.0;
    qreal m_slowMultiplier = 1.0;
    qreal m_slowDuration = 0.0;
    QColor m_color;
    bool m_finished = false;
};

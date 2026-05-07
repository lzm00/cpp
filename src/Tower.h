#pragma once

#include "GameObject.h"

#include <QColor>
#include <QGraphicsItem>
#include <QPointF>
#include <QString>

class Enemy;
class GameScene;

enum class TowerType
{
    Archer,
    Mage,
    Support
};

class Tower : public QGraphicsItem, public GameObject
{
public:
    Tower(TowerType type, const QPointF &position);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void updateObject(qreal dt) override;
    void updateTower(GameScene *scene, qreal dt);

    bool canUpgrade() const;
    void upgrade();
    int level() const;
    int upgradeCost() const;
    TowerType towerType() const;
    int damage() const;
    qreal slowMultiplier() const;
    qreal slowDuration() const;

    static int buildCost(TowerType type);
    static QString typeName(TowerType type);

private:
    Enemy *selectTarget(GameScene *scene) const;
    QColor baseColor() const;

    TowerType m_type;
    int m_level = 1;
    int m_damage = 24;
    qreal m_range = 112.0;
    qreal m_fireInterval = 0.48;
    qreal m_cooldownRemaining = 0.0;
    qreal m_slowMultiplier = 1.0;
    qreal m_slowDuration = 0.0;
};

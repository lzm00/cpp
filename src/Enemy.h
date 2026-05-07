#pragma once

#include "GameObject.h"

#include <QObject>
#include <QGraphicsItem>
#include <QVector>
#include <QPointF>

enum class EnemyType
{
    Grunt,
    Tank,
    Assassin
};

class Enemy : public QObject, public QGraphicsItem, public GameObject
{
public:
    Enemy(EnemyType type, const QVector<QPointF> &path, int waveIndex);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void updateObject(qreal dt) override;

    void applyDamage(int amount);
    void applySlow(qreal multiplier, qreal duration);
    void freezeFor(int durationMs);
    void shieldFor(int durationMs);
    void speedBoostFor(int durationMs, qreal multiplier);
    void setPath(const QVector<QPointF> &path);

    bool isDead() const;
    bool reachedEnd() const;
    int reward() const;
    int baseDamage() const;
    qreal hpRatio() const;
    qreal pathProgress() const;
    EnemyType enemyType() const;

private:
    QVector<QPointF> m_path;
    EnemyType m_type;
    int m_waypointIndex = 1;
    int m_maxHp = 100;
    int m_hp = 100;
    int m_reward = 10;
    int m_baseDamage = 1;
    qreal m_size = 28.0;
    qreal m_speed = 70.0;
    qreal m_slowMultiplier = 1.0;
    qreal m_slowTime = 0.0;
    bool isFrozen = false;
    bool isShielded = false;
    bool isSpeedBoosted = false;
    qreal m_speedBoostMultiplier = 1.0;
    int m_freezeToken = 0;
    int m_shieldToken = 0;
    int m_speedBoostToken = 0;
    bool m_reachedEnd = false;
};

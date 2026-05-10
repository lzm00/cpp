#pragma once

#include "GameObject.h"

#include <QObject>
#include <QGraphicsItem>
#include <QVector>
#include <QPointF>

// 敌人类型：普通敌人、坦克敌人、快速敌人。
// 讲解点：EnemyType 定义敌人种类，不同类型会在 Enemy 构造函数里获得不同属性。
enum class EnemyType
{
    Grunt,
    Tank,
    Assassin,
    Priest,
    Boss
};

// Enemy 同时是 Qt 图形项和游戏对象：负责绘制、移动、受伤和状态效果。
// 讲解点：Enemy 同时是可绘制对象和可更新对象，负责移动、受伤、状态效果和到达基地判断。
class Enemy : public QObject, public QGraphicsItem, public GameObject
{
public:
    Enemy(EnemyType type, const QVector<QPointF> &path, int waveIndex);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void updateObject(qreal dt) override;

    void applyDamage(int amount);
    void heal(int amount);
    void applySlow(qreal multiplier, qreal duration);
    void applyBurn(int damagePerTick, int ticks, int intervalMs);
    void applyVulnerability(qreal multiplier, qreal duration);
    void freezeFor(int durationMs);
    void shieldFor(int durationMs);
    void speedBoostFor(int durationMs, qreal multiplier);
    void updateTerrainState(bool inRiver, bool inBush);
    void setPath(const QVector<QPointF> &path);
    void setWaypointIndex(int index);

    bool isDead() const;
    bool reachedEnd() const;
    bool isHidden() const;
    int reward() const;
    int baseDamage() const;
    qreal hpRatio() const;
    qreal pathProgress() const;
    int waypointIndex() const;
    bool specialReady() const;
    void resetSpecialCooldown(qreal seconds);
    EnemyType enemyType() const;

private:
    // 路径和当前位置进度。
    QVector<QPointF> m_path;
    EnemyType m_type;
    int m_waypointIndex = 1;

    // 基础属性：生命、奖励、伤害、尺寸、速度。
    int m_maxHp = 100;
    int m_hp = 100;
    int m_reward = 10;
    int m_baseDamage = 1;
    qreal m_size = 28.0;
    qreal m_speed = 70.0;
    qreal m_slowMultiplier = 1.0;
    qreal m_slowTime = 0.0;
    qreal m_vulnerabilityMultiplier = 1.0;
    qreal m_vulnerabilityTime = 0.0;
    qreal m_specialCooldown = 0.0;
    qreal m_terrainSpeedMultiplier = 1.0;
    qreal m_hiddenTime = 0.0;

    // 临时状态效果。
    bool isFrozen = false;
    bool isShielded = false;
    bool isSpeedBoosted = false;
    qreal m_speedBoostMultiplier = 1.0;

    // token 用来避免旧定时器把新的状态提前取消。
    int m_freezeToken = 0;
    int m_shieldToken = 0;
    int m_speedBoostToken = 0;
    bool m_reachedEnd = false;
};

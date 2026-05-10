#pragma once

#include "GameObject.h"

#include <QColor>
#include <QGraphicsItem>
#include <QPointF>
#include <QString>

class Enemy;
class GameScene;

// 防御塔类型：弓箭塔、法师塔、辅助塔。
// 讲解点：TowerType 定义三类防御塔：射手、法师、辅助。
enum class TowerType
{
    Archer,
    Mage,
    Support
};

enum class TowerBranch
{
    None,
    ArcherSpeed,
    ArcherCritical,
    MageExplosion,
    MageBurn,
    SupportSlow,
    SupportVulnerability
};

// Tower 负责寻找目标、按冷却时间攻击，以及升级自身属性。
// 讲解点：Tower 负责冷却计时、选择目标、发射子弹，以及升级和分支强化。
class Tower : public QGraphicsItem, public GameObject
{
public:
    Tower(TowerType type, const QPointF &position);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void updateObject(qreal dt) override;
    void updateTower(GameScene *scene, qreal dt);

    bool canUpgrade() const;
    bool needsBranchChoice() const;
    void upgrade(TowerBranch branch = TowerBranch::None);
    int level() const;
    int upgradeCost() const;
    TowerType towerType() const;
    int damage() const;
    qreal slowMultiplier() const;
    qreal slowDuration() const;
    TowerBranch branch() const;
    qreal range() const;
    qreal critChance() const;
    qreal critMultiplier() const;
    qreal explosionRadius() const;
    qreal explosionDamageRatio() const;
    int burnDamage() const;
    int burnTicks() const;
    int burnIntervalMs() const;
    qreal vulnerabilityMultiplier() const;
    qreal vulnerabilityDuration() const;

    static int buildCost(TowerType type);
    static QString typeName(TowerType type);
    static QString branchName(TowerBranch branch);

private:
    // 从射程内的敌人里选出最靠近终点的目标。
    Enemy *selectTarget(GameScene *scene) const;
    QColor baseColor() const;

    // 炮塔属性：等级、伤害、射程、攻击间隔和辅助减速效果。
    TowerType m_type;
    TowerBranch m_branch = TowerBranch::None;
    int m_level = 1;
    int m_damage = 24;
    qreal m_range = 112.0;
    qreal m_fireInterval = 0.48;
    qreal m_cooldownRemaining = 0.0;
    qreal m_slowMultiplier = 1.0;
    qreal m_slowDuration = 0.0;
    qreal m_critChance = 0.0;
    qreal m_critMultiplier = 2.0;
    qreal m_explosionRadius = 0.0;
    qreal m_explosionDamageRatio = 0.0;
    int m_burnDamage = 0;
    int m_burnTicks = 0;
    int m_burnIntervalMs = 450;
    qreal m_vulnerabilityMultiplier = 1.0;
    qreal m_vulnerabilityDuration = 0.0;
};

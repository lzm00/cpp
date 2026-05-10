#include "Tower.h"

#include "Enemy.h"
#include "GameScene.h"

#include <QAudioOutput>
#include <QLineF>
#include <QMediaPlayer>
#include <QPainter>
#include <QPixmap>
#include <QUrl>
#include <QtMath>

namespace
{
// 根据炮塔类型选择图片素材；图片缺失时，paint() 会画备用图形。
QPixmap towerIcon(TowerType type)
{
    static const QPixmap archerIcon(QStringLiteral("D:/cpp/picture/52e89a65835d02850972baac4f243193.jpg"));
    static const QPixmap mageIcon(QStringLiteral("D:/cpp/picture/0f655f924aaa3ad4a65908c7cc4ce424.jpg"));
    static const QPixmap supportIcon(QStringLiteral("D:/cpp/picture/db481a2e05459caa531541cfffb37ae6.jpg"));

    switch (type) {
    case TowerType::Archer:
        return archerIcon;
    case TowerType::Mage:
        return mageIcon;
    case TowerType::Support:
        return supportIcon;
    }
    return QPixmap();
}

// 法师攻击时临时创建一个播放器，播放完毕后自动释放。
void playMageAttackSound()
{
    auto *player = new QMediaPlayer;
    auto *audioOutput = new QAudioOutput(player);
    audioOutput->setVolume(0.85);
    player->setAudioOutput(audioOutput);
    player->setSource(QUrl::fromLocalFile(R"(D:\cpp\picture\2099726dae614ab928ce91b3bb8cbc98.mp4)"));
    QObject::connect(player, &QMediaPlayer::mediaStatusChanged, player, [player](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia || status == QMediaPlayer::InvalidMedia) {
            player->deleteLater();
        }
    });
    QObject::connect(player, &QMediaPlayer::errorOccurred, player, [player](QMediaPlayer::Error, const QString &) {
        player->deleteLater();
    });
    player->play();
}
}

Tower::Tower(TowerType type, const QPointF &position)
    : m_type(type)
{
    // 讲解点：Tower 根据类型初始化攻击方式，射手快、法师伤害高、辅助带减速。
    // 不同炮塔类型对应不同的初始属性。
    switch (type) {
    case TowerType::Archer:
        m_damage = 22;
        m_range = 126.0;
        m_fireInterval = 0.38;
        break;
    case TowerType::Mage:
        m_damage = 58;
        m_range = 118.0;
        m_fireInterval = 0.95;
        break;
    case TowerType::Support:
        m_damage = 12;
        m_range = 132.0;
        m_fireInterval = 0.62;
        m_slowMultiplier = 0.55;
        m_slowDuration = 1.35;
        break;
    }

    (void)towerIcon(m_type);
    m_fireInterval *= 0.75;

    setPos(position);
    setZValue(20);
}

QRectF Tower::boundingRect() const
{
    return QRectF(-28, -30, 56, 60);
}

void Tower::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QPixmap icon = towerIcon(m_type);
    if (!icon.isNull()) {
        // 优先使用图片素材绘制炮塔，并在右下角显示等级。
        painter->setPen(QPen(QColor(70, 62, 54), 2));
        painter->setBrush(QColor(54, 48, 43, 190));
        painter->drawEllipse(QRectF(-25, -24, 50, 46));
        painter->drawPixmap(QRectF(-24, -30, 48, 48),
                            icon,
                            QRectF(QPointF(0, 0), QSizeF(icon.size())));

        painter->setPen(QPen(QColor(57, 52, 48), 2));
        painter->setBrush(QColor(255, 251, 225));
        painter->drawEllipse(QRectF(10, 10, 16, 16));
        painter->setPen(QColor(57, 52, 48));
        painter->drawText(QRectF(10, 10, 16, 16), Qt::AlignCenter, QString::number(m_level));
        return;
    }

    // 图片缺失时，用 QPainter 画出不同类型的备用炮塔外观。
    painter->setPen(QPen(QColor(70, 62, 54), 2));
    painter->setBrush(QColor(92, 83, 71));
    painter->drawRoundedRect(QRectF(-18, -8, 36, 28), 5, 5);

    painter->setBrush(baseColor());
    switch (m_type) {
    case TowerType::Archer:
        painter->drawPolygon(QPolygonF({QPointF(-18, -6), QPointF(0, -26), QPointF(18, -6)}));
        painter->setPen(QPen(QColor(245, 238, 207), 3));
        painter->drawLine(QPointF(0, -22), QPointF(0, 4));
        break;
    case TowerType::Mage:
        painter->drawEllipse(QRectF(-15, -25, 30, 30));
        painter->setBrush(QColor(250, 230, 140));
        painter->drawEllipse(QRectF(-6, -16, 12, 12));
        break;
    case TowerType::Support:
        painter->drawRoundedRect(QRectF(-16, -24, 32, 22), 6, 6);
        painter->setPen(QPen(QColor(235, 247, 232), 3));
        painter->drawLine(QPointF(-8, -13), QPointF(8, -13));
        painter->drawLine(QPointF(0, -21), QPointF(0, -5));
        break;
    }

    painter->setPen(QPen(QColor(57, 52, 48), 2));
    painter->setBrush(QColor(255, 251, 225));
    painter->drawEllipse(QRectF(10, 10, 16, 16));
    painter->setPen(QColor(57, 52, 48));
    painter->drawText(QRectF(10, 10, 16, 16), Qt::AlignCenter, QString::number(m_level));
}

void Tower::updateObject(qreal dt)
{
    m_cooldownRemaining = qMax<qreal>(0.0, m_cooldownRemaining - dt);
}

void Tower::updateTower(GameScene *scene, qreal dt)
{
    // 讲解点：防御塔每帧更新冷却，冷却结束后在射程内选择最接近基地的敌人攻击。
    updateObject(dt);

    if (m_branch == TowerBranch::SupportVulnerability && m_vulnerabilityMultiplier > 1.0) {
        const auto enemies = scene->enemiesInRange(pos(), m_range);
        for (Enemy *enemy : enemies) {
            if (enemy && !enemy->isDead()) {
                enemy->applyVulnerability(m_vulnerabilityMultiplier, m_vulnerabilityDuration);
            }
        }
    }

    if (m_cooldownRemaining > 0.0) {
        return;
    }

    // 冷却结束后选择目标，有目标才发射投射物。
    Enemy *target = selectTarget(scene);
    if (!target) {
        return;
    }

    scene->launchProjectile(this, target);
    if (m_type == TowerType::Mage) {
        playMageAttackSound();
    }
    m_cooldownRemaining = m_fireInterval;
}

bool Tower::canUpgrade() const
{
    return m_level < 5;
}

bool Tower::needsBranchChoice() const
{
    return m_level == 3 && m_branch == TowerBranch::None;
}

void Tower::upgrade(TowerBranch branch)
{
    // 讲解点：防御塔升级会提升属性，3 级之后可以选择不同强化分支。
    if (!canUpgrade()) {
        return;
    }

    if (needsBranchChoice()) {
        m_branch = branch;
    }

    ++m_level;
    if (m_level <= 3) {
        m_damage = qRound(m_damage * 1.35);
        m_range += 14.0;
        m_fireInterval *= 0.90;
    } else {
        switch (m_branch) {
        case TowerBranch::ArcherSpeed:
            m_damage = qRound(m_damage * (m_level == 4 ? 1.15 : 1.20));
            m_fireInterval *= (m_level == 4 ? 0.58 : 0.50);
            m_range += 8.0;
            break;
        case TowerBranch::ArcherCritical:
            m_damage = qRound(m_damage * (m_level == 4 ? 1.25 : 1.30));
            m_critChance = (m_level == 4 ? 0.25 : 0.40);
            m_critMultiplier = 2.0;
            m_range += 10.0;
            break;
        case TowerBranch::MageExplosion:
            m_damage = qRound(m_damage * (m_level == 4 ? 1.18 : 1.25));
            m_explosionRadius = (m_level == 4 ? 78.0 : 104.0);
            m_explosionDamageRatio = (m_level == 4 ? 0.45 : 0.65);
            m_range += 12.0;
            break;
        case TowerBranch::MageBurn:
            m_damage = qRound(m_damage * (m_level == 4 ? 1.12 : 1.18));
            m_burnDamage = qRound(m_damage * (m_level == 4 ? 0.20 : 0.28));
            m_burnTicks = (m_level == 4 ? 3 : 5);
            m_burnIntervalMs = 450;
            m_fireInterval *= 0.92;
            break;
        case TowerBranch::SupportSlow:
            m_damage = qRound(m_damage * 1.12);
            m_slowMultiplier = (m_level == 4 ? 0.40 : 0.30);
            m_slowDuration = (m_level == 4 ? 1.95 : 2.55);
            m_range += 18.0;
            break;
        case TowerBranch::SupportVulnerability:
            m_damage = qRound(m_damage * (m_level == 4 ? 1.20 : 1.25));
            m_vulnerabilityMultiplier = (m_level == 4 ? 1.22 : 1.38);
            m_vulnerabilityDuration = 0.20;
            m_range += 16.0;
            break;
        case TowerBranch::None:
            m_damage = qRound(m_damage * 1.20);
            m_range += 10.0;
            break;
        }
    }
    // 升级会提升伤害和射程，并缩短攻击间隔。
    ++m_level;
    update();
}

int Tower::level() const
{
    return m_level;
}

int Tower::upgradeCost() const
{
    const int base = buildCost(m_type) / 2 + 120;
    return base * m_level * (m_level + 1);
}

TowerType Tower::towerType() const
{
    return m_type;
}

int Tower::damage() const
{
    return m_damage;
}

qreal Tower::slowMultiplier() const
{
    return m_slowMultiplier;
}

qreal Tower::slowDuration() const
{
    return m_slowDuration;
}

TowerBranch Tower::branch() const
{
    return m_branch;
}

qreal Tower::range() const
{
    return m_range;
}

qreal Tower::critChance() const
{
    return m_critChance;
}

qreal Tower::critMultiplier() const
{
    return m_critMultiplier;
}

qreal Tower::explosionRadius() const
{
    return m_explosionRadius;
}

qreal Tower::explosionDamageRatio() const
{
    return m_explosionDamageRatio;
}

int Tower::burnDamage() const
{
    return m_burnDamage;
}

int Tower::burnTicks() const
{
    return m_burnTicks;
}

int Tower::burnIntervalMs() const
{
    return m_burnIntervalMs;
}

qreal Tower::vulnerabilityMultiplier() const
{
    return m_vulnerabilityMultiplier;
}

qreal Tower::vulnerabilityDuration() const
{
    return m_vulnerabilityDuration;
}

int Tower::buildCost(TowerType type)
{
    switch (type) {
    case TowerType::Archer:
        return 225;
    case TowerType::Mage:
        return 350;
    case TowerType::Support:
        return 275;
    }
    return 225;
}

QString Tower::typeName(TowerType type)
{
    switch (type) {
    case TowerType::Archer:
        return QStringLiteral("射手塔");
    case TowerType::Mage:
        return QStringLiteral("法师塔");
    case TowerType::Support:
        return QStringLiteral("辅助塔");
    }
    return QStringLiteral("防御塔");
}

QString Tower::branchName(TowerBranch branch)
{
    switch (branch) {
    case TowerBranch::ArcherSpeed:
        return QStringLiteral("攻速路线");
    case TowerBranch::ArcherCritical:
        return QStringLiteral("暴击路线");
    case TowerBranch::MageExplosion:
        return QStringLiteral("爆炸路线");
    case TowerBranch::MageBurn:
        return QStringLiteral("灼烧路线");
    case TowerBranch::SupportSlow:
        return QStringLiteral("减速路线");
    case TowerBranch::SupportVulnerability:
        return QStringLiteral("增伤路线");
    case TowerBranch::None:
        return QStringLiteral("未选择分支");
    }
    return QStringLiteral("未选择分支");
}

Enemy *Tower::selectTarget(GameScene *scene) const
{
    // 讲解点：索敌策略是优先攻击路径进度最大的敌人，也就是最接近基地的敌人。
    const auto enemies = scene->enemiesInRange(pos(), m_range + scene->towerRangeBonusAt(pos()));
    Enemy *best = nullptr;
    qreal bestProgress = -1.0;

    // 优先攻击路径进度最大的敌人，也就是更接近基地的敌人。
    for (Enemy *enemy : enemies) {
        if (!enemy || enemy->isDead() || enemy->isHidden()) {
            continue;
        }
        const qreal progress = enemy->pathProgress();
        if (progress > bestProgress) {
            bestProgress = progress;
            best = enemy;
        }
    }

    return best;
}

QColor Tower::baseColor() const
{
    switch (m_type) {
    case TowerType::Archer:
        return QColor(92, 153, 103);
    case TowerType::Mage:
        return QColor(125, 102, 176);
    case TowerType::Support:
        return QColor(82, 154, 176);
    }
    return QColor(92, 153, 103);
}

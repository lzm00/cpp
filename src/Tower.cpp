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
QPixmap towerIcon(TowerType type)
{
    QString path;
    switch (type) {
    case TowerType::Archer:
        path = QStringLiteral("D:/cpp/picture/52e89a65835d02850972baac4f243193.jpg");
        break;
    case TowerType::Mage:
        path = QStringLiteral("D:/cpp/picture/0f655f924aaa3ad4a65908c7cc4ce424.jpg");
        break;
    case TowerType::Support:
        path = QStringLiteral("D:/cpp/picture/db481a2e05459caa531541cfffb37ae6.jpg");
        break;
    }
    return QPixmap(path);
}

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
    updateObject(dt);
    if (m_cooldownRemaining > 0.0) {
        return;
    }

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

void Tower::upgrade()
{
    if (!canUpgrade()) {
        return;
    }

    ++m_level;
    m_damage = qRound(m_damage * 1.45);
    m_range += 16.0;
    m_fireInterval *= 0.86;
    update();
}

int Tower::level() const
{
    return m_level;
}

int Tower::upgradeCost() const
{
    return (buildCost(m_type) / 2 + 80) * m_level * 3;
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

Enemy *Tower::selectTarget(GameScene *scene) const
{
    const auto enemies = scene->enemiesInRange(pos(), m_range);
    Enemy *best = nullptr;
    qreal bestProgress = -1.0;

    for (Enemy *enemy : enemies) {
        if (!enemy || enemy->isDead()) {
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

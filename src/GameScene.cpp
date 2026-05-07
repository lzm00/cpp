#include "GameScene.h"

#include "Enemy.h"
#include "Projectile.h"

#include <QAudioOutput>
#include <QDebug>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsVideoItem>
#include <QLineF>
#include <QMediaPlayer>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QTransform>
#include <QUrl>

#include <utility>

GameScene::GameScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setupMap();
    loadBackground();

    connect(&m_timer, &QTimer::timeout, this, &GameScene::gameLoop);
    m_timer.start(30);

    emit statsChanged(m_gold, m_lives, m_wave, m_maxWaves);
}

QList<Enemy *> GameScene::enemiesInRange(const QPointF &center, qreal range) const
{
    QList<Enemy *> result;
    for (Enemy *enemy : m_enemies) {
        if (!enemy || enemy->isDead() || enemy->reachedEnd()) {
            continue;
        }
        if (QLineF(center, enemy->pos()).length() <= range) {
            result.append(enemy);
        }
    }
    return result;
}

void GameScene::launchProjectile(Tower *tower, Enemy *target)
{
    if (!tower || !target) {
        return;
    }

    auto *projectile = new Projectile(tower->pos(),
                                      target,
                                      tower->damage(),
                                      tower->slowMultiplier(),
                                      tower->slowDuration(),
                                      projectileColor(tower->towerType()));
    m_projectiles.append(projectile);
    addItem(projectile);
}

int GameScene::gold() const
{
    return m_gold;
}

int GameScene::lives() const
{
    return m_lives;
}

int GameScene::wave() const
{
    return m_wave;
}

int GameScene::maxWaves() const
{
    return m_maxWaves;
}

void GameScene::startNextWave()
{
    if (m_gameOver) {
        emit messageChanged(QStringLiteral("本局已经结束，请重新开始。"));
        return;
    }

    if (hasActiveWave()) {
        emit messageChanged(QStringLiteral("当前波次尚未清空。"));
        return;
    }

    if (m_wave >= m_maxWaves) {
        finishGame(true, QStringLiteral("峡谷守卫成功！"));
        return;
    }

    ++m_wave;
    m_spawnedThisWave = 0;
    m_spawnQueue.clear();
    const int enemyCount = 8 + m_wave * 4;
    for (int i = 0; i < enemyCount; ++i) {
        if (i % 7 == 6) {
            m_spawnQueue.append(EnemyType::Tank);
        } else if (i % 4 == 3) {
            m_spawnQueue.append(EnemyType::Assassin);
        } else {
            m_spawnQueue.append(EnemyType::Grunt);
        }
    }
    m_spawnClock = 0.0;
    emit statsChanged(m_gold, m_lives, m_wave, m_maxWaves);
    emit messageChanged(QStringLiteral("第 %1 波敌人正在进攻。").arg(m_wave));
}

void GameScene::resetGame()
{
    for (Projectile *projectile : std::as_const(m_projectiles)) {
        removeItem(projectile);
        delete projectile;
    }
    m_projectiles.clear();

    for (Enemy *enemy : std::as_const(m_enemies)) {
        removeItem(enemy);
        delete enemy;
    }
    m_enemies.clear();

    for (Tower *tower : std::as_const(m_towers)) {
        removeItem(tower);
        delete tower;
    }
    m_towers.clear();

    for (BuildSpot &spot : m_buildSpots) {
        spot.tower = nullptr;
    }

    m_gold = 1000;
    m_lives = 12;
    m_wave = 0;
    m_spawnQueue.clear();
    m_spawnedThisWave = 0;
    m_spawnClock = 0.0;
    m_gameOver = false;
    if (!m_timer.isActive()) {
        m_timer.start(30);
    }

    update();
    emit statsChanged(m_gold, m_lives, m_wave, m_maxWaves);
    emit messageChanged(QStringLiteral("选择塔并点击建塔点，准备守卫峡谷。"));
}

void GameScene::setSelectedTowerType(TowerType type)
{
    m_selectedTowerType = type;
    emit messageChanged(QStringLiteral("已选择 %1，造价 %2 金币。")
                            .arg(Tower::typeName(type))
                            .arg(Tower::buildCost(type)));
    update();
}

void GameScene::freezeAllEnemies()
{
    for (Enemy *enemy : std::as_const(m_enemies)) {
        if (enemy && !enemy->isDead() && !enemy->reachedEnd()) {
            enemy->freezeFor(750);
        }
    }
}

void GameScene::shieldAllEnemies()
{
    for (Enemy *enemy : std::as_const(m_enemies)) {
        if (enemy && !enemy->isDead() && !enemy->reachedEnd()) {
            enemy->shieldFor(1500);
        }
    }
}

void GameScene::speedBoostAllEnemies()
{
    for (Enemy *enemy : std::as_const(m_enemies)) {
        if (enemy && !enemy->isDead() && !enemy->reachedEnd()) {
            enemy->speedBoostFor(2500, 1.6);
        }
    }
}

void GameScene::triggerCrystalExplosionFailure()
{
    if (m_gameOver) {
        return;
    }

    playBaseHitEffect();
    finishGame(false, QStringLiteral("Blue crystal exploded."));
}

void GameScene::drawBackground(QPainter *painter, const QRectF &)
{
    if (!m_backgroundItem) {
        painter->fillRect(sceneRect(), QColor(38, 35, 32));
        painter->setPen(QColor(255, 220, 170));
        painter->drawText(sceneRect(),
                          Qt::AlignCenter,
                          QStringLiteral("背景图片加载失败\n:/assets/valley_map.png"));
    }
}

void GameScene::drawForeground(QPainter *painter, const QRectF &)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    const int cost = Tower::buildCost(m_selectedTowerType);

    for (const BuildSpot &spot : m_buildSpots) {
        if (spot.tower) {
            continue;
        }

        const bool affordable = m_gold >= cost;
        painter->setPen(QPen(affordable ? QColor(61, 117, 80) : QColor(130, 79, 73), 2, Qt::DashLine));
        painter->setBrush(affordable ? QColor(238, 246, 222, 170) : QColor(242, 219, 210, 160));
        painter->drawEllipse(spot.position, 23, 23);

        painter->setPen(QColor(65, 60, 53));
        painter->drawText(QRectF(spot.position.x() - 22, spot.position.y() - 9, 44, 18),
                          Qt::AlignCenter,
                          QString::number(cost));
    }
}

void GameScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    qDebug() << "Clicked scene position:" << event->scenePos();

    if (m_gameOver || event->button() != Qt::LeftButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    QGraphicsItem *clicked = itemAt(event->scenePos(), QTransform());
    if (auto *tower = dynamic_cast<Tower *>(clicked)) {
        if (!tower->canUpgrade()) {
            emit messageChanged(QStringLiteral("%1 已达到最高等级。").arg(Tower::typeName(tower->towerType())));
            return;
        }

        const int cost = tower->upgradeCost();
        if (m_gold < cost) {
            emit messageChanged(QStringLiteral("升级需要 %1 金币，当前金币不足。").arg(cost));
            return;
        }

        m_gold -= cost;
        tower->upgrade();
        emit statsChanged(m_gold, m_lives, m_wave, m_maxWaves);
        emit messageChanged(QStringLiteral("%1 升至 %2 级。")
                                .arg(Tower::typeName(tower->towerType()))
                                .arg(tower->level()));
        return;
    }

    const int spotIndex = findBuildSpot(event->scenePos());
    if (spotIndex < 0) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    BuildSpot &spot = m_buildSpots[spotIndex];
    if (spot.tower) {
        return;
    }

    const int cost = Tower::buildCost(m_selectedTowerType);
    if (m_gold < cost) {
        emit messageChanged(QStringLiteral("金币不足，无法建造 %1。").arg(Tower::typeName(m_selectedTowerType)));
        return;
    }

    auto *tower = new Tower(m_selectedTowerType, spot.position);
    spot.tower = tower;
    m_towers.append(tower);
    addItem(tower);
    m_gold -= cost;

    update();
    emit statsChanged(m_gold, m_lives, m_wave, m_maxWaves);
    emit messageChanged(QStringLiteral("建造了 %1。点击它可以升级。").arg(Tower::typeName(m_selectedTowerType)));
}

void GameScene::setupMap()
{
    enemySpawnPoint = QPointF(1195, 260);
    playerBasePoint = QPointF(270, 880);

    topLanePath = {
        enemySpawnPoint,
        QPointF(1080, 215),
        QPointF(865, 165),
        QPointF(620, 160),
        QPointF(420, 175),
        QPointF(250, 275),
        QPointF(225, 465),
        QPointF(230, 675),
        playerBasePoint
    };

    middleLanePath = {
        enemySpawnPoint,
        QPointF(1040, 325),
        QPointF(880, 430),
        QPointF(720, 540),
        QPointF(560, 650),
        QPointF(400, 760),
        playerBasePoint
    };

    bottomLanePath = {
        enemySpawnPoint,
        QPointF(1255, 390),
        QPointF(1260, 565),
        QPointF(1225, 740),
        QPointF(1040, 880),
        QPointF(760, 940),
        QPointF(520, 920),
        playerBasePoint
    };

    m_buildSpots = {
        {QPointF(227, 388), nullptr},
        {QPointF(229, 656), nullptr},
        {QPointF(243, 760), nullptr},
        {QPointF(402, 807), nullptr},
        {QPointF(525, 752), nullptr},
        {QPointF(602, 616), nullptr},
        {QPointF(495, 165), nullptr},
        {QPointF(646, 941), nullptr},
        {QPointF(815, 165), nullptr},
        {QPointF(880, 482), nullptr},
        {QPointF(937, 382), nullptr},
        {QPointF(989, 954), nullptr},
        {QPointF(1026, 165), nullptr},
        {QPointF(1085, 324), nullptr},
        {QPointF(1189, 224), nullptr},
        {QPointF(1250, 384), nullptr},
        {QPointF(1265, 515), nullptr},
        {QPointF(1268, 733), nullptr},
        {QPointF(480, 958), nullptr}
    };
}

void GameScene::loadBackground()
{
    const QPixmap bg(QStringLiteral(":/assets/valley_map.png"));
    if (bg.isNull()) {
        qDebug() << "Failed to load background image from Qt resource:"
                 << ":/assets/valley_map.png";
        setSceneRect(0, 0, 1000, 750);
        return;
    }

    m_backgroundItem = addPixmap(bg);
    m_backgroundItem->setZValue(-100);
    m_backgroundItem->setPos(0, 0);
    setSceneRect(bg.rect());
}

void GameScene::gameLoop()
{
    if (m_gameOver) {
        return;
    }

    constexpr qreal dt = 0.030;

    if (!m_spawnQueue.isEmpty()) {
        m_spawnClock -= dt;
        if (m_spawnClock <= 0.0) {
            spawnEnemy();
            m_spawnClock = qMax<qreal>(0.35, 0.82 - m_wave * 0.04);
        }
    }

    for (Enemy *enemy : std::as_const(m_enemies)) {
        enemy->updateObject(dt);
    }

    for (Tower *tower : std::as_const(m_towers)) {
        tower->updateTower(this, dt);
    }

    for (Projectile *projectile : std::as_const(m_projectiles)) {
        projectile->updateObject(dt);
    }

    bool statsDirty = false;
    for (int i = m_projectiles.size() - 1; i >= 0; --i) {
        if (m_projectiles.at(i)->isFinished()) {
            removeProjectileAt(i);
        }
    }

    for (int i = m_enemies.size() - 1; i >= 0; --i) {
        Enemy *enemy = m_enemies.at(i);
        if (enemy->isDead()) {
            m_gold += enemy->reward();
            removeEnemyAt(i);
            statsDirty = true;
        } else if (enemy->reachedEnd()) {
            m_lives -= enemy->baseDamage();
            playBaseHitEffect();
            removeEnemyAt(i);
            statsDirty = true;
        }
    }

    if (statsDirty) {
        emit statsChanged(m_gold, qMax(0, m_lives), m_wave, m_maxWaves);
    }

    if (m_lives <= 0) {
        finishGame(false, QStringLiteral("基地生命值归零，守卫失败。"));
        return;
    }

    if (m_wave >= m_maxWaves && !hasActiveWave()) {
        finishGame(true, QStringLiteral("所有波次已清空，峡谷守卫成功！"));
    }
}

void GameScene::spawnEnemy()
{
    if (m_spawnQueue.isEmpty()) {
        return;
    }
    const EnemyType type = m_spawnQueue.takeFirst();

    QVector<QPointF> selectedPath;
    switch ((m_spawnedThisWave + m_wave) % 3) {
    case 0:
        selectedPath = topLanePath;
        break;
    case 1:
        selectedPath = middleLanePath;
        break;
    default:
        selectedPath = bottomLanePath;
        break;
    }

    auto *enemy = new Enemy(type, selectedPath, m_wave);
    enemy->setPos(enemySpawnPoint);
    enemy->setPath(selectedPath);
    m_enemies.append(enemy);
    addItem(enemy);
    ++m_spawnedThisWave;
}

void GameScene::playBaseHitEffect()
{
    constexpr qreal effectSize = 118.0;
    auto *effectItem = new QGraphicsVideoItem;
    effectItem->setSize(QSizeF(effectSize, effectSize));
    effectItem->setPos(playerBasePoint - QPointF(effectSize / 2.0, effectSize / 2.0));
    effectItem->setZValue(90);
    addItem(effectItem);

    auto *player = new QMediaPlayer(this);
    auto *audioOutput = new QAudioOutput(player);
    audioOutput->setVolume(1.0);
    player->setVideoOutput(effectItem);
    player->setAudioOutput(audioOutput);
    player->setSource(QUrl::fromLocalFile(R"(D:\cpp\picture\3758bcf24622cb2294090749e90e8c94.mp4)"));

    connect(player, &QMediaPlayer::mediaStatusChanged, this, [this, player, effectItem](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia || status == QMediaPlayer::InvalidMedia) {
            removeItem(effectItem);
            delete effectItem;
            player->deleteLater();
        }
    });
    connect(player, &QMediaPlayer::errorOccurred, this, [this, player, effectItem](QMediaPlayer::Error, const QString &errorString) {
        qDebug() << "Base hit effect video error:" << errorString;
        removeItem(effectItem);
        delete effectItem;
        player->deleteLater();
    });

    player->play();
}

void GameScene::removeEnemyAt(int index)
{
    Enemy *enemy = m_enemies.takeAt(index);
    removeItem(enemy);
    delete enemy;
}

void GameScene::removeProjectileAt(int index)
{
    Projectile *projectile = m_projectiles.takeAt(index);
    removeItem(projectile);
    delete projectile;
}

void GameScene::finishGame(bool victory, const QString &message)
{
    if (m_gameOver) {
        return;
    }

    m_gameOver = true;
    m_spawnQueue.clear();
    m_timer.stop();
    emit statsChanged(m_gold, qMax(0, m_lives), m_wave, m_maxWaves);
    emit messageChanged(message);
    emit gameFinished(victory, message);
}

int GameScene::findBuildSpot(const QPointF &point) const
{
    for (int i = 0; i < m_buildSpots.size(); ++i) {
        if (QLineF(point, m_buildSpots.at(i).position).length() <= 26.0) {
            return i;
        }
    }
    return -1;
}

bool GameScene::hasActiveWave() const
{
    return !m_spawnQueue.isEmpty() || !m_enemies.isEmpty() || !m_projectiles.isEmpty();
}

QColor GameScene::projectileColor(TowerType type) const
{
    switch (type) {
    case TowerType::Archer:
        return QColor(235, 214, 107);
    case TowerType::Mage:
        return QColor(184, 135, 222);
    case TowerType::Support:
        return QColor(116, 211, 219);
    }
    return QColor(235, 214, 107);
}

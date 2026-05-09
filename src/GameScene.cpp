#include "GameScene.h"

#include "Enemy.h"
#include "Projectile.h"

#include <QAudioOutput>
#include <QDebug>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsVideoItem>
#include <QGraphicsView>
#include <QLineF>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTransform>
#include <QUrl>

#include <utility>

GameScene::GameScene(QObject *parent)
    : QGraphicsScene(parent)
{
    // 构造场景时先准备地图，再启动定时器驱动游戏循环。
    setupMap();
    loadBackground();

    connect(&m_timer, &QTimer::timeout, this, &GameScene::gameLoop);
    m_timer.start(30);

    emit statsChanged(m_gold, m_lives, m_wave, m_maxWaves);
}

QList<Enemy *> GameScene::enemiesInRange(const QPointF &center, qreal range) const
{
    // 给炮塔查询射程内的有效敌人。
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

    // 炮塔攻击时创建子弹，并交给场景后续更新和删除。
    auto *projectile = new Projectile(tower->pos(),
                                      target,
                                      tower->damage(),
                                      tower->slowMultiplier(),
                                      tower->slowDuration(),
                                      tower->critChance(),
                                      tower->critMultiplier(),
                                      tower->explosionRadius(),
                                      tower->explosionDamageRatio(),
                                      tower->burnDamage(),
                                      tower->burnTicks(),
                                      tower->burnIntervalMs(),
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

int GameScene::killCount() const
{
    return m_killCount;
}

int GameScene::leakedCount() const
{
    return m_leakedCount;
}

int GameScene::skillUseCount() const
{
    return m_skillUseCount;
}

int GameScene::highestTowerLevel() const
{
    return m_highestTowerLevel;
}

int GameScene::completedWaveCount() const
{
    return m_completedWaves;
}

qreal GameScene::towerRangeBonusAt(const QPointF &position) const
{
    return containsAny(m_highlandAreas, position) ? 42.0 : 0.0;
}

void GameScene::startNextWave()
{
    // 每次开始新波次前，先检查游戏是否结束或上一波是否还没清完。
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
    // 波次越高，敌人数量越多；队列里混入坦克和快速敌人。
    // Three lanes spawn on the same cadence, with distinct lane identities.
    const int topLaneCount = 3 + m_wave;
    const int middleLaneCount = 7 + m_wave * 3;
    const int bottomLaneCount = 5 + m_wave * 2;

    for (int i = 0; i < topLaneCount; ++i) {
        if (m_wave >= 6 && i % 8 == 7) {
            m_spawnQueue.append(EnemyType::Priest);
        } else {
            m_spawnQueue.append((i % 5 == 4) ? EnemyType::Grunt : EnemyType::Tank);
        }
    }
    for (int i = 0; i < middleLaneCount; ++i) {
        if (m_wave >= 4 && i % 9 == 8) {
            m_spawnQueue.append(EnemyType::Priest);
        } else if (i % 10 == 9) {
            m_spawnQueue.append(EnemyType::Tank);
        } else if (i % 5 == 4) {
            m_spawnQueue.append(EnemyType::Assassin);
        } else {
            m_spawnQueue.append(EnemyType::Grunt);
        }
    }
    for (int i = 0; i < bottomLaneCount; ++i) {
        if (m_wave >= 8 && i % 10 == 9) {
            m_spawnQueue.append(EnemyType::Priest);
        } else {
            m_spawnQueue.append((i % 6 == 5) ? EnemyType::Grunt : EnemyType::Assassin);
        }
    }
    if (m_wave % 5 == 0) {
        m_spawnQueue.prepend(EnemyType::Boss);
    }
    m_spawnClock = 0.0;
    emit statsChanged(m_gold, m_lives, m_wave, m_maxWaves);
    emit messageChanged(QStringLiteral("第 %1 波敌人正在进攻。").arg(m_wave));
}

void GameScene::resetGame()
{
    // 重开时删除所有场景对象，并把资源和波次恢复到初始状态。
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
    m_lives = 10;
    m_wave = 0;
    m_spawnQueue.clear();
    m_spawnedThisWave = 0;
    m_killCount = 0;
    m_leakedCount = 0;
    m_skillUseCount = 0;
    m_highestTowerLevel = 0;
    m_completedWaves = 0;
    m_spawnClock = 0.0;
    m_crystalShieldActive = false;
    m_hasMingDao = false;
    m_mingDaoTriggered = false;
    m_gamePaused = false;
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
    if (!m_gameOver) {
        ++m_skillUseCount;
    }
    for (Enemy *enemy : std::as_const(m_enemies)) {
        if (enemy && !enemy->isDead() && !enemy->reachedEnd()) {
            enemy->freezeFor(750);
        }
    }
}

void GameScene::shieldAllEnemies()
{
    if (!m_gameOver) {
        ++m_skillUseCount;
    }
    for (Enemy *enemy : std::as_const(m_enemies)) {
        if (enemy && !enemy->isDead() && !enemy->reachedEnd()) {
            enemy->shieldFor(1500);
        }
    }
}

void GameScene::speedBoostAllEnemies()
{
    if (!m_gameOver) {
        ++m_skillUseCount;
    }
    for (Enemy *enemy : std::as_const(m_enemies)) {
        if (enemy && !enemy->isDead() && !enemy->reachedEnd()) {
            enemy->speedBoostFor(2500, 1.6);
        }
    }
}

void GameScene::activateCrystalShield()
{
    if (m_gameOver) {
        return;
    }

    ++m_skillUseCount;
    m_crystalShieldActive = true;
    emit messageChanged(QStringLiteral("水晶护盾已启动，5 秒内不会掉血。"));
    QTimer::singleShot(5000, this, [this]() {
        m_crystalShieldActive = false;
        if (!m_gameOver) {
            emit messageChanged(QStringLiteral("水晶护盾结束。"));
        }
    });
}

void GameScene::activateCrystalShockwave()
{
    if (m_gameOver) {
        return;
    }

    ++m_skillUseCount;
    constexpr qreal shockwaveRadius = 180.0;
    int removedCount = 0;
    for (int i = m_enemies.size() - 1; i >= 0; --i) {
        Enemy *enemy = m_enemies.at(i);
        if (!enemy || enemy->isDead() || enemy->reachedEnd()) {
            continue;
        }
        if (QLineF(playerBasePoint, enemy->pos()).length() <= shockwaveRadius) {
            removeEnemyAt(i);
            ++m_killCount;
            ++removedCount;
        }
    }

    emit messageChanged(QStringLiteral("水晶冲击波清除了 %1 个近身敌人。").arg(removedCount));
}

void GameScene::activateTimeFreeze()
{
    if (m_gameOver) {
        return;
    }

    ++m_skillUseCount;
    for (Enemy *enemy : std::as_const(m_enemies)) {
        if (enemy && !enemy->isDead() && !enemy->reachedEnd()) {
            enemy->freezeFor(1000);
        }
    }
    emit messageChanged(QStringLiteral("时间冻结，所有敌人停止移动 1 秒。"));
}

bool GameScene::activateCrystalLifeSaver()
{
    if (m_gameOver) {
        return false;
    }

    constexpr int lifeSaverCost = 1000;
    if (m_gamePaused) {
        emit messageChanged(QStringLiteral("游戏暂停中，暂时不能使用名刀司命。"));
        return false;
    }

    if (m_mingDaoTriggered) {
        emit messageChanged(QStringLiteral("名刀司命本局已经触发过，无法再次生效。"));
        return false;
    }

    if (m_hasMingDao) {
        emit messageChanged(QStringLiteral("名刀司命已生效，触发前不能重复购买。"));
        return false;
    }

    if (m_gold < lifeSaverCost) {
        emit messageChanged(QStringLiteral("金币不足，保命装备需要 %1 金币。").arg(lifeSaverCost));
        return false;
    }

    m_gold -= lifeSaverCost;
    m_hasMingDao = true;
    ++m_skillUseCount;
    emit statsChanged(m_gold, m_lives, m_wave, m_maxWaves);
    emit messageChanged(QStringLiteral("名刀司命已生效：水晶最后 1 点生命时可免疫一次致命伤害。"));
    return true;
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
    Q_UNUSED(painter);
}

void GameScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    qDebug() << "Clicked scene position:" << event->scenePos();

    // 鼠标左键点击炮塔时尝试升级；点击空建塔点时尝试建塔。
    if (m_gameOver || event->button() != Qt::LeftButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    QGraphicsItem *clicked = itemAt(event->scenePos(), QTransform());
    if (auto *tower = dynamic_cast<Tower *>(clicked)) {
        if (!tower->canUpgrade()) {
            emit messageChanged(QString::fromUtf8("\xE5\xB7\xB2\xE6\xBB\xA1\xE7\xBA\xA7"));
            return;
            emit messageChanged(QStringLiteral("已满级"));
            return;
            emit messageChanged(QStringLiteral("%1 已达到最高等级。").arg(Tower::typeName(tower->towerType())));
            return;
        }

        const int cost = tower->upgradeCost();
        if (m_gold < cost) {
            emit messageChanged(QStringLiteral("升级需要 %1 金币，当前金币不足。").arg(cost));
            return;
        }

        TowerBranch branch = TowerBranch::None;
        if (tower->needsBranchChoice()) {
            branch = chooseUpgradeBranch(tower);
            if (branch == TowerBranch::None) {
                return;
            }
        }

        m_gold -= cost;
        tower->upgrade(branch);
        m_highestTowerLevel = qMax(m_highestTowerLevel, tower->level());
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
    m_highestTowerLevel = qMax(m_highestTowerLevel, tower->level());
    m_gold -= cost;

    update();
    emit statsChanged(m_gold, m_lives, m_wave, m_maxWaves);
    emit messageChanged(QStringLiteral("建造了 %1。点击它可以升级。").arg(Tower::typeName(m_selectedTowerType)));
}

void GameScene::setupMap()
{
    // 地图坐标都基于背景图像素位置：出生点、基地、三条路线和建塔点。
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

    m_riverAreas = {
        QRectF(610, 455, 350, 170),
        QRectF(1120, 520, 160, 230)
    };

    m_bushAreas = {
        QRectF(835, 120, 285, 155),
        QRectF(1010, 790, 280, 145)
    };

    m_highlandAreas = {
        QRectF(430, 105, 230, 145),
        QRectF(860, 315, 260, 135),
        QRectF(455, 850, 250, 130)
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
    // 背景通过 resources.qrc 注册为 Qt 资源，路径以 :/assets 开头。
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
    if (m_gameOver || m_gamePaused) {
        return;
    }

    // 这里假设定时器每 30ms 触发一次，所以 dt 固定为 0.030 秒。
    constexpr qreal dt = 0.030;

    // 按间隔从刷怪队列中生成敌人。
    if (!m_spawnQueue.isEmpty()) {
        m_spawnClock -= dt;
        if (m_spawnClock <= 0.0) {
            spawnEnemies();
            m_spawnClock = 0.30;
        }
    }

    for (Enemy *enemy : std::as_const(m_enemies)) {
        if (enemy) {
            enemy->updateTerrainState(containsAny(m_riverAreas, enemy->pos()),
                                      containsAny(m_bushAreas, enemy->pos()));
        }
        enemy->updateObject(dt);
    }

    handleSpecialEnemies();

    for (Tower *tower : std::as_const(m_towers)) {
        tower->updateTower(this, dt);
    }

    for (Projectile *projectile : std::as_const(m_projectiles)) {
        projectile->updateObject(dt);
    }

    // 从后往前删除已完成子弹和死亡/到达终点的敌人，避免索引错位。
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
            ++m_killCount;
            removeEnemyAt(i);
            statsDirty = true;
        } else if (enemy->reachedEnd()) {
            ++m_leakedCount;
            if (!m_crystalShieldActive) {
                if (m_lives <= 1 && m_hasMingDao && !m_mingDaoTriggered) {
                    removeEnemyAt(i);
                    emit statsChanged(m_gold, qMax(0, m_lives), m_wave, m_maxWaves);
                    triggerMingDao();
                    return;
                }

                if (m_lives <= 1 && m_mingDaoTriggered) {
                    m_lives = 0;
                    removeEnemyAt(i);
                    emit statsChanged(m_gold, qMax(0, m_lives), m_wave, m_maxWaves);
                    finishGame(false, QStringLiteral("名刀司命已失效，基地生命值归零，守卫失败。"));
                    return;
                }

                m_lives -= enemy->baseDamage();
                playBaseHitEffect();
            }
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

    if (!hasActiveWave()) {
        m_completedWaves = qMax(m_completedWaves, m_wave);
    }

    if (m_wave >= m_maxWaves && !hasActiveWave()) {
        finishGame(true, QStringLiteral("所有波次已清空，峡谷守卫成功！"));
    }
}

void GameScene::pauseGame()
{
    if (m_gameOver || m_gamePaused) {
        return;
    }

    m_gamePaused = true;
    m_timer.stop();
}

void GameScene::resumeGame()
{
    if (m_gameOver || !m_gamePaused) {
        return;
    }

    m_gamePaused = false;
    if (!m_timer.isActive()) {
        m_timer.start(30);
    }
}

void GameScene::triggerMingDao()
{
    m_hasMingDao = false;
    m_mingDaoTriggered = true;
    pauseGame();
    emit messageChanged(QStringLiteral("名刀司命触发，水晶免疫了这次致命伤害。"));
    playMingDaoHitVideo();
}

void GameScene::spawnEnemies()
{
    if (m_spawnQueue.isEmpty()) {
        return;
    }

    const EnemyType type = m_spawnQueue.takeFirst();
    const QVector<QPointF> &path = randomLanePath();
    auto *enemy = new Enemy(type, path, m_wave);
    enemy->setPos(enemySpawnPoint);
    enemy->setPath(path);
    if (type == EnemyType::Boss && m_wave == 10) {
        enemy->shieldFor(6500);
    }
    m_enemies.append(enemy);
    addItem(enemy);
    ++m_spawnedThisWave;
}

const QVector<QPointF> &GameScene::randomLanePath() const
{
    const int lane = QRandomGenerator::global()->bounded(3);
    if (lane == 0) {
        return topLanePath;
    }
    if (lane == 1) {
        return middleLanePath;
    }
    return bottomLanePath;
}

bool GameScene::containsAny(const QVector<QRectF> &areas, const QPointF &point) const
{
    for (const QRectF &area : areas) {
        if (area.contains(point)) {
            return true;
        }
    }
    return false;
}

void GameScene::handleSpecialEnemies()
{
    QList<Enemy *> bossesToSummon;
    for (Enemy *enemy : std::as_const(m_enemies)) {
        if (!enemy || enemy->isDead() || enemy->reachedEnd() || !enemy->specialReady()) {
            continue;
        }

        if (enemy->enemyType() == EnemyType::Priest) {
            constexpr qreal priestRadius = 130.0;
            int affectedCount = 0;
            for (Enemy *ally : std::as_const(m_enemies)) {
                if (!ally || ally == enemy || ally->isDead() || ally->reachedEnd()) {
                    continue;
                }
                if (QLineF(enemy->pos(), ally->pos()).length() > priestRadius) {
                    continue;
                }

                if (ally->hpRatio() < 0.85) {
                    ally->heal(45 + m_wave * 8);
                } else {
                    ally->shieldFor(900);
                }
                ++affectedCount;
            }
            enemy->resetSpecialCooldown(3.4);
            if (affectedCount > 0) {
                enemy->shieldFor(450);
            }
        } else if (enemy->enemyType() == EnemyType::Boss && m_wave == 15) {
            bossesToSummon.append(enemy);
            enemy->resetSpecialCooldown(5.5);
        }
    }

    for (Enemy *boss : std::as_const(bossesToSummon)) {
        spawnSummonedEnemy(boss);
        spawnSummonedEnemy(boss);
    }
}

void GameScene::spawnSummonedEnemy(Enemy *boss)
{
    if (!boss) {
        return;
    }

    const QVector<QPointF> &path = randomLanePath();
    auto *minion = new Enemy(EnemyType::Grunt, path, m_wave);
    minion->setPath(path);
    minion->setWaypointIndex(boss->waypointIndex());
    minion->setPos(boss->pos() + QPointF(18.0 - 36.0 * (m_spawnedThisWave % 2), 20.0));
    m_enemies.append(minion);
    addItem(minion);
    ++m_spawnedThisWave;
}

void GameScene::playBaseHitEffect()
{
    // 敌人到达基地时，在基地位置播放一次视频特效。
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

void GameScene::playMingDaoHitVideo()
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
            playMingDaoSoundAndResume();
        }
    });
    connect(player, &QMediaPlayer::errorOccurred, this, [this, player, effectItem](QMediaPlayer::Error, const QString &errorString) {
        qDebug() << "Ming Dao hit video error:" << errorString;
        removeItem(effectItem);
        delete effectItem;
        player->deleteLater();
        playMingDaoSoundAndResume();
    });

    player->play();
}

void GameScene::playMingDaoSoundAndResume()
{
    auto *player = new QMediaPlayer(this);
    auto *audioOutput = new QAudioOutput(player);
    audioOutput->setVolume(1.0);
    player->setAudioOutput(audioOutput);
    player->setSource(QUrl::fromLocalFile(R"(D:\cpp\picture\23cac5b3ec1ea97dd74467f3b80313a4.mp4)"));

    connect(player, &QMediaPlayer::mediaStatusChanged, this, [this, player](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia || status == QMediaPlayer::InvalidMedia) {
            player->deleteLater();
            resumeGame();
        }
    });
    connect(player, &QMediaPlayer::errorOccurred, this, [this, player](QMediaPlayer::Error, const QString &errorString) {
        qDebug() << "Ming Dao sound error:" << errorString;
        player->deleteLater();
        resumeGame();
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

    // 停止刷怪和主循环，再通过信号通知主窗口播放结算表现。
    m_gameOver = true;
    m_spawnQueue.clear();
    m_timer.stop();
    emit statsChanged(m_gold, qMax(0, m_lives), m_wave, m_maxWaves);
    emit messageChanged(message);
    emit gameFinished(victory, message);
}

TowerBranch GameScene::chooseUpgradeBranch(Tower *tower) const
{
    if (!tower) {
        return TowerBranch::None;
    }

    TowerBranch firstBranch = TowerBranch::None;
    TowerBranch secondBranch = TowerBranch::None;
    QString firstText;
    QString secondText;
    QString detailText;

    switch (tower->towerType()) {
    case TowerType::Archer:
        firstBranch = TowerBranch::ArcherSpeed;
        secondBranch = TowerBranch::ArcherCritical;
        firstText = QStringLiteral("攻速路线");
        secondText = QStringLiteral("暴击路线");
        detailText = QStringLiteral("射手塔选择分支强化");
        break;
    case TowerType::Mage:
        firstBranch = TowerBranch::MageExplosion;
        secondBranch = TowerBranch::MageBurn;
        firstText = QStringLiteral("爆炸路线");
        secondText = QStringLiteral("灼烧路线");
        detailText = QStringLiteral("法师塔选择分支强化");
        break;
    case TowerType::Support:
        firstBranch = TowerBranch::SupportSlow;
        secondBranch = TowerBranch::SupportVulnerability;
        firstText = QStringLiteral("减速路线");
        secondText = QStringLiteral("增伤路线");
        detailText = QStringLiteral("辅助塔选择分支强化");
        break;
    }

    QWidget *parentWidget = views().isEmpty() ? nullptr : views().first();
    QMessageBox box(parentWidget);
    box.setWindowTitle(QStringLiteral("分支强化"));
    box.setText(detailText);
    box.setInformativeText(QStringLiteral("选择后该塔会沿此路线升到 5 级。"));
    QPushButton *firstButton = box.addButton(firstText, QMessageBox::AcceptRole);
    QPushButton *secondButton = box.addButton(secondText, QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Cancel);
    box.exec();

    if (box.clickedButton() == firstButton) {
        return firstBranch;
    }
    if (box.clickedButton() == secondButton) {
        return secondBranch;
    }
    return TowerBranch::None;
}

int GameScene::findBuildSpot(const QPointF &point) const
{
    // 鼠标点击离建塔点足够近时，认为选中了该建塔点。
    for (int i = 0; i < m_buildSpots.size(); ++i) {
        if (QLineF(point, m_buildSpots.at(i).position).length() <= 26.0) {
            return i;
        }
    }
    return -1;
}

bool GameScene::hasActiveWave() const
{
    return !m_spawnQueue.isEmpty()
           || !m_enemies.isEmpty()
           || !m_projectiles.isEmpty();
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

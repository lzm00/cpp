#pragma once

#include "Enemy.h"
#include "Tower.h"

#include <QGraphicsScene>
#include <QRectF>
#include <QTimer>
#include <QVector>

class QGraphicsPixmapItem;
class Projectile;

// 可建塔位置：position 是地图坐标，tower 为空表示还没建塔。
struct BuildSpot
{
    QPointF position;
    Tower *tower = nullptr;
};

// GameScene 是游戏核心：管理地图、敌人、炮塔、子弹、波次和胜负判定。
class GameScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit GameScene(QObject *parent = nullptr);

    QList<Enemy *> enemiesInRange(const QPointF &center, qreal range) const;
    void launchProjectile(Tower *tower, Enemy *target);

    int gold() const;
    int lives() const;
    int wave() const;
    int maxWaves() const;
    int killCount() const;
    int leakedCount() const;
    int skillUseCount() const;
    int highestTowerLevel() const;
    int completedWaveCount() const;
    qreal towerRangeBonusAt(const QPointF &position) const;

public slots:
    void startNextWave();
    void resetGame();
    void setSelectedTowerType(TowerType type);
    void freezeAllEnemies();
    void shieldAllEnemies();
    void speedBoostAllEnemies();
    void activateCrystalShield();
    void activateCrystalShockwave();
    void activateTimeFreeze();
    void triggerCrystalExplosionFailure();
    bool activateCrystalLifeSaver();

signals:
    void statsChanged(int gold, int lives, int wave, int maxWaves);
    void messageChanged(const QString &message);
    void gameFinished(bool victory, const QString &message);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    // 初始化地图路径和建塔点。
    void setupMap();
    void loadBackground();

    // 主循环相关函数。
    void gameLoop();
    void spawnEnemies();
    void handleSpecialEnemies();
    void spawnSummonedEnemy(Enemy *boss);
    const QVector<QPointF> &randomLanePath() const;
    bool containsAny(const QVector<QRectF> &areas, const QPointF &point) const;
    void pauseGame();
    void resumeGame();
    void triggerMingDao();
    void playMingDaoHitVideo();
    void playMingDaoSoundAndResume();
    void playBaseHitEffect();
    void removeEnemyAt(int index);
    void removeProjectileAt(int index);
    void finishGame(bool victory, const QString &message);
    TowerBranch chooseUpgradeBranch(Tower *tower) const;
    int findBuildSpot(const QPointF &point) const;
    bool hasActiveWave() const;
    QColor projectileColor(TowerType type) const;

    // 定时器驱动 gameLoop，每 30ms 更新一次游戏状态。
    QTimer m_timer;
    QGraphicsPixmapItem *m_backgroundItem = nullptr;

    // 地图关键点和三条敌人行进路径。
    QPointF enemySpawnPoint;
    QPointF playerBasePoint;
    QVector<QPointF> topLanePath;
    QVector<QPointF> middleLanePath;
    QVector<QPointF> bottomLanePath;
    QVector<QRectF> m_riverAreas;
    QVector<QRectF> m_bushAreas;
    QVector<QRectF> m_highlandAreas;
    QVector<BuildSpot> m_buildSpots;

    // 当前场景里的活动对象。
    QList<Enemy *> m_enemies;
    QList<Tower *> m_towers;
    QList<Projectile *> m_projectiles;
    QList<EnemyType> m_spawnQueue;

    // 游戏状态和资源。
    TowerType m_selectedTowerType = TowerType::Archer;
    int m_gold = 1000;
    int m_lives = 10;
    int m_wave = 0;
    int m_maxWaves = 10;
    int m_spawnedThisWave = 0;
    int m_killCount = 0;
    int m_leakedCount = 0;
    int m_skillUseCount = 0;
    int m_highestTowerLevel = 0;
    int m_completedWaves = 0;
    qreal m_spawnClock = 0.0;
    bool m_crystalShieldActive = false;
    bool m_hasMingDao = false;
    bool m_mingDaoTriggered = false;
    bool m_gamePaused = false;
    bool m_gameOver = false;
};

#pragma once

#include "Enemy.h"
#include "Tower.h"

#include <QGraphicsScene>
#include <QTimer>
#include <QVector>

class QGraphicsPixmapItem;
class Projectile;

struct BuildSpot
{
    QPointF position;
    Tower *tower = nullptr;
};

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

public slots:
    void startNextWave();
    void resetGame();
    void setSelectedTowerType(TowerType type);
    void freezeAllEnemies();
    void shieldAllEnemies();
    void speedBoostAllEnemies();
    void triggerCrystalExplosionFailure();

signals:
    void statsChanged(int gold, int lives, int wave, int maxWaves);
    void messageChanged(const QString &message);
    void gameFinished(bool victory, const QString &message);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    void setupMap();
    void loadBackground();
    void gameLoop();
    void spawnEnemy();
    void playBaseHitEffect();
    void removeEnemyAt(int index);
    void removeProjectileAt(int index);
    void finishGame(bool victory, const QString &message);
    int findBuildSpot(const QPointF &point) const;
    bool hasActiveWave() const;
    QColor projectileColor(TowerType type) const;

    QTimer m_timer;
    QGraphicsPixmapItem *m_backgroundItem = nullptr;
    QPointF enemySpawnPoint;
    QPointF playerBasePoint;
    QVector<QPointF> topLanePath;
    QVector<QPointF> middleLanePath;
    QVector<QPointF> bottomLanePath;
    QVector<BuildSpot> m_buildSpots;
    QList<Enemy *> m_enemies;
    QList<Tower *> m_towers;
    QList<Projectile *> m_projectiles;
    QList<EnemyType> m_spawnQueue;

    TowerType m_selectedTowerType = TowerType::Archer;
    int m_gold = 1000;
    int m_lives = 12;
    int m_wave = 0;
    int m_maxWaves = 20;
    int m_spawnedThisWave = 0;
    qreal m_spawnClock = 0.0;
    bool m_gameOver = false;
};

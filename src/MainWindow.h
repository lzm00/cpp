#pragma once

#include "Tower.h"

#include <QMainWindow>

class GameScene;
class QAudioOutput;
class QLabel;
class QMediaPlayer;
class QPushButton;
class QStackedLayout;
class QVideoWidget;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void updateStats(int gold, int lives, int wave, int maxWaves);
    void showGameFinished(bool victory, const QString &message);
    void restartAfterResultVideo();
    void handleStartButtonClicked();

private:
    QPushButton *createTowerButton(TowerType type);
    void selectTower(TowerType type);
    QWidget *createVideoOverlay(QWidget *parent);
    void playIntroVideo();
    void playResultVideo(bool victory);
    void playStartSound();
    void playBattleSong();
    void playFirstToolbarSound();
    void playSecondToolbarSound();
    void startBackgroundMusic();
    void stopBackgroundMusic();
    void showResultButtons();
    void hideResultVideo();

    GameScene *m_scene = nullptr;
    QStackedLayout *m_stackLayout = nullptr;
    QWidget *m_gamePage = nullptr;
    QWidget *m_videoOverlay = nullptr;
    QWidget *m_resultButtonPanel = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
    QMediaPlayer *m_videoPlayer = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QMediaPlayer *m_startSoundPlayer = nullptr;
    QAudioOutput *m_startAudioOutput = nullptr;
    QMediaPlayer *m_battleSongPlayer = nullptr;
    QAudioOutput *m_battleSongAudioOutput = nullptr;
    QMediaPlayer *m_firstToolbarPlayer = nullptr;
    QAudioOutput *m_firstToolbarAudioOutput = nullptr;
    QMediaPlayer *m_secondToolbarPlayer = nullptr;
    QAudioOutput *m_secondToolbarAudioOutput = nullptr;
    QMediaPlayer *m_backgroundMusicPlayer = nullptr;
    QAudioOutput *m_backgroundAudioOutput = nullptr;
    QLabel *m_goldIconLabel = nullptr;
    QLabel *m_goldLabel = nullptr;
    QLabel *m_livesLabel = nullptr;
    QLabel *m_waveLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
    QPushButton *m_archerButton = nullptr;
    QPushButton *m_mageButton = nullptr;
    QPushButton *m_supportButton = nullptr;
    bool m_resultVideoMode = false;
};

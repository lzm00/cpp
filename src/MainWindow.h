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

// 主窗口负责搭建界面、连接按钮和游戏场景，并控制视频/音频播放。
// 讲解点：MainWindow 是界面控制中心，负责显示地图、右侧面板、视频和音频，并把操作转发给 GameScene。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void updateStats(int gold, int lives, int wave, int maxWaves);
    void showGameFinished(bool victory, const QString &message);
    void restartAfterResultVideo();
    void handleStartButtonClicked();

private:
    // UI 交互辅助函数。
    QPushButton *createTowerButton(TowerType type);
    void selectTower(TowerType type);
    QWidget *createVideoOverlay(QWidget *parent);

    // 视频和音频播放控制。
    void playIntroVideo();
    void playResultVideo(bool victory);
    void playStartSound();
    void playBattleSong();
    void playSecondBattleSong();
    void playFirstToolbarSound();
    void playSecondToolbarSound();
    void enterGameFromIntro(bool startFirstWave);
    void updateIntroStartButtonGeometry();
    void startBackgroundMusic();
    void stopBackgroundMusic();
    void showResultButtons();
    void hideResultVideo();

    // 游戏页面和场景。
    GameScene *m_scene = nullptr;
    QStackedLayout *m_stackLayout = nullptr;
    QWidget *m_gamePage = nullptr;

    // 视频遮罩层和结果按钮。
    QWidget *m_videoOverlay = nullptr;
    QWidget *m_resultButtonPanel = nullptr;
    QLabel *m_resultStatsLabel = nullptr;
    QPushButton *m_introStartButton = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
    QMediaPlayer *m_videoPlayer = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QMediaPlayer *m_startSoundPlayer = nullptr;
    QAudioOutput *m_startAudioOutput = nullptr;
    QMediaPlayer *m_battleSongPlayer = nullptr;
    QAudioOutput *m_battleSongAudioOutput = nullptr;
    QMediaPlayer *m_secondBattleSongPlayer = nullptr;
    QAudioOutput *m_secondBattleSongAudioOutput = nullptr;
    QMediaPlayer *m_firstToolbarPlayer = nullptr;
    QAudioOutput *m_firstToolbarAudioOutput = nullptr;
    QMediaPlayer *m_secondToolbarPlayer = nullptr;
    QAudioOutput *m_secondToolbarAudioOutput = nullptr;
    QMediaPlayer *m_backgroundMusicPlayer = nullptr;
    QAudioOutput *m_backgroundAudioOutput = nullptr;

    // 右侧信息栏控件。
    QLabel *m_goldIconLabel = nullptr;
    QLabel *m_goldLabel = nullptr;
    QLabel *m_livesLabel = nullptr;
    QLabel *m_waveLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
    QPushButton *m_archerButton = nullptr;
    QPushButton *m_mageButton = nullptr;
    QPushButton *m_supportButton = nullptr;
    bool m_resultVideoMode = false;
    bool m_introVideoMode = false;
};

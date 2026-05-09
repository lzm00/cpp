#include "MainWindow.h"

#include "GameScene.h"

#include <QAudioOutput>
#include <QButtonGroup>
#include <QDebug>
#include <QEvent>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsView>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>
#include <QStackedLayout>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoWidget>

namespace
{
QRect introVideoContentRect(const QRect &widgetRect)
{
    constexpr qreal videoAspectRatio = 16.0 / 9.0;
    const qreal widgetAspectRatio = widgetRect.width() / qMax<qreal>(1.0, widgetRect.height());
    QSizeF contentSize;
    if (widgetAspectRatio > videoAspectRatio) {
        contentSize.setHeight(widgetRect.height());
        contentSize.setWidth(contentSize.height() * videoAspectRatio);
    } else {
        contentSize.setWidth(widgetRect.width());
        contentSize.setHeight(contentSize.width() / videoAspectRatio);
    }

    const QPointF topLeft(widgetRect.center().x() - contentSize.width() / 2.0,
                          widgetRect.center().y() - contentSize.height() / 2.0);
    return QRectF(topLeft, contentSize).toAlignedRect();
}
}

class AutoFitGraphicsView : public QGraphicsView
{
public:
    explicit AutoFitGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr)
        : QGraphicsView(scene, parent)
    {
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        // 窗口尺寸变化时，保持整张地图按比例缩放显示。
        QGraphicsView::resizeEvent(event);
        if (scene()) {
            fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
        }
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_scene(new GameScene(this))
{
    // 主界面由左侧游戏地图和右侧控制面板组成。
    setWindowTitle(QStringLiteral("峡谷守卫战 Valley Guardian TD"));
    resize(1260, 800);

    auto *central = new QWidget(this);
    m_stackLayout = new QStackedLayout(central);
    m_stackLayout->setContentsMargins(0, 0, 0, 0);
    m_stackLayout->setStackingMode(QStackedLayout::StackAll);

    m_gamePage = new QWidget(central);
    auto *rootLayout = new QHBoxLayout(m_gamePage);
    rootLayout->setContentsMargins(4, 4, 4, 4);
    rootLayout->setSpacing(6);

    auto *view = new AutoFitGraphicsView(m_scene, m_gamePage);
    // QGraphicsView 负责把 GameScene 显示出来。
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setMinimumSize(940, 704);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setFrameShape(QFrame::NoFrame);
    view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    view->setBackgroundBrush(QColor(221, 229, 207));
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    rootLayout->addWidget(view, 1);

    auto *panel = new QWidget(m_gamePage);
    // 右侧面板显示资源、波次、建塔按钮和技能按钮。
    panel->setFixedWidth(208);
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(6, 4, 6, 4);
    panelLayout->setSpacing(6);

    auto *title = new QLabel(QStringLiteral("峡谷守卫战"), panel);
    title->setObjectName(QStringLiteral("titleLabel"));
    panelLayout->addWidget(title);

    auto *goldLayout = new QHBoxLayout();
    goldLayout->setContentsMargins(0, 0, 0, 0);
    goldLayout->setSpacing(6);
    m_goldIconLabel = new QLabel(panel);
    m_goldIconLabel->setFixedSize(28, 28);
    m_goldIconLabel->setPixmap(QPixmap(QStringLiteral("D:/cpp/picture/69015e4ca4b3627fda6d0bd15e9caf6b.jpg"))
                                   .scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_goldLabel = new QLabel(panel);
    m_livesLabel = new QLabel(panel);
    m_waveLabel = new QLabel(panel);
    goldLayout->addWidget(m_goldIconLabel);
    goldLayout->addWidget(m_goldLabel, 1);
    panelLayout->addLayout(goldLayout);
    panelLayout->addWidget(m_livesLabel);
    panelLayout->addWidget(m_waveLabel);

    auto *line = new QFrame(panel);
    line->setFrameShape(QFrame::HLine);
    panelLayout->addWidget(line);

    auto *towerGroup = new QButtonGroup(this);
    towerGroup->setExclusive(true);

    m_archerButton = createTowerButton(TowerType::Archer);
    m_mageButton = createTowerButton(TowerType::Mage);
    m_supportButton = createTowerButton(TowerType::Support);
    m_archerButton->setChecked(true);

    towerGroup->addButton(m_archerButton);
    towerGroup->addButton(m_mageButton);
    towerGroup->addButton(m_supportButton);
    panelLayout->addWidget(m_archerButton);
    panelLayout->addWidget(m_mageButton);
    panelLayout->addWidget(m_supportButton);

    connect(m_archerButton, &QPushButton::clicked, this, [this]() { selectTower(TowerType::Archer); });
    connect(m_mageButton, &QPushButton::clicked, this, [this]() { selectTower(TowerType::Mage); });
    connect(m_supportButton, &QPushButton::clicked, this, [this]() { selectTower(TowerType::Support); });

    auto *startButton = new QPushButton(QStringLiteral("开始波次"), panel);
    auto *resetButton = new QPushButton(QStringLiteral("重新开始"), panel);
    auto *battleSongButton = new QPushButton(QStringLiteral("战歌1"), panel);
    auto *secondBattleSongButton = new QPushButton(QStringLiteral("战歌2"), panel);
    auto *firstToolbarButton = new QPushButton(panel);
    auto *secondToolbarButton = new QPushButton(panel);
    auto *freezeSkillButton = new QPushButton(panel);
    auto *shieldSkillButton = new QPushButton(panel);
    auto *speedSkillButton = new QPushButton(panel);
    auto *crystalLifeSaverButton = new QPushButton(panel);
    auto *crystalShieldButton = new QPushButton(QStringLiteral("水晶护盾"), panel);
    auto *crystalShockwaveButton = new QPushButton(QStringLiteral("水晶冲击波"), panel);
    auto *timeFreezeButton = new QPushButton(QStringLiteral("时间冻结"), panel);
    auto *crystalFailureButton = new QPushButton(panel);
    startButton->setObjectName(QStringLiteral("startImageButton"));
    startButton->setText(QString());
    startButton->setIcon(QIcon(QStringLiteral("D:/cpp/picture/5ea6379ab94e66baf2f8b796827a890c.jpg")));
    startButton->setIconSize(QSize(180, 54));
    startButton->setMinimumHeight(58);
    resetButton->setMinimumHeight(30);
    battleSongButton->setMinimumHeight(30);
    secondBattleSongButton->setMinimumHeight(30);
    crystalShieldButton->setMinimumHeight(30);
    crystalShockwaveButton->setMinimumHeight(30);
    timeFreezeButton->setMinimumHeight(30);
    crystalShieldButton->setToolTip(QStringLiteral("蓝色水晶 5 秒内不掉血，冷却 20 秒"));
    crystalShockwaveButton->setToolTip(QStringLiteral("清除蓝色水晶附近 180 范围内敌人，冷却 25 秒"));
    timeFreezeButton->setToolTip(QStringLiteral("所有敌人停止移动 1 秒，冷却 18 秒"));
    firstToolbarButton->setObjectName(QStringLiteral("sideImageButton"));
    firstToolbarButton->setText(QString());
    firstToolbarButton->setIcon(QIcon(QStringLiteral(R"(D:\cpp\picture\2b3a92dd9c19b0ab73c17b0a37d802a9.jpg)")));
    firstToolbarButton->setIconSize(QSize(56, 56));
    firstToolbarButton->setMinimumHeight(58);
    secondToolbarButton->setObjectName(QStringLiteral("sideImageButton"));
    secondToolbarButton->setText(QString());
    secondToolbarButton->setIcon(QIcon(QStringLiteral(R"(D:\cpp\picture\1cee33fb4583fcc34daab53941e5db6e.jpg)")));
    secondToolbarButton->setIconSize(QSize(56, 56));
    secondToolbarButton->setMinimumHeight(58);
    auto setupSkillButton = [](QPushButton *button, const QString &imagePath, const QString &tooltip) {
        // 统一设置图片技能按钮的外观。
        button->setObjectName(QStringLiteral("skillImageButton"));
        button->setText(QString());
        button->setToolTip(tooltip);
        button->setIcon(QIcon(imagePath));
        button->setIconSize(QSize(56, 56));
        button->setFixedSize(62, 62);
    };
    setupSkillButton(freezeSkillButton,
                     QStringLiteral(R"(D:\cpp\picture\c87057edd7cdf3a6ce46ac93484f8a42.jpg)"),
                     QStringLiteral("Freeze all enemies for 0.75s"));
    setupSkillButton(shieldSkillButton,
                     QStringLiteral(R"(D:\cpp\picture\34c15160218fa1a64369f4e500e9ac81.jpg)"),
                     QStringLiteral("Shield all enemies for 1.5s"));
    setupSkillButton(speedSkillButton,
                     QStringLiteral(R"(D:\cpp\picture\3d97863343d67f73b2a4fa84f9d155fc.jpg)"),
                     QStringLiteral("Speed up all enemies for 2.5s"));
    setupSkillButton(crystalLifeSaverButton,
                     QStringLiteral(R"(D:\cpp\picture\2927b939faaf4832ff58d42f16259c78.jpg)"),
                     QStringLiteral("名刀司命：花费 1000 金币获得一次水晶致命伤害免疫，冷却 20 秒"));
    setupSkillButton(firstToolbarButton,
                     QStringLiteral(R"(D:\cpp\picture\2b3a92dd9c19b0ab73c17b0a37d802a9.jpg)"),
                     QStringLiteral("Play first toolbar sound"));
    setupSkillButton(secondToolbarButton,
                     QStringLiteral(R"(D:\cpp\picture\1cee33fb4583fcc34daab53941e5db6e.jpg)"),
                     QStringLiteral("Play second toolbar sound"));
    setupSkillButton(crystalFailureButton,
                     QStringLiteral(R"(D:\cpp\picture\a6b474cd1da0626ce272b8a2c563d9fd.jpg)"),
                     QStringLiteral("Explode the blue crystal"));

    auto *imageButtonGrid = new QGridLayout();
    imageButtonGrid->setContentsMargins(0, 0, 0, 0);
    imageButtonGrid->setHorizontalSpacing(5);
    imageButtonGrid->setVerticalSpacing(5);
    imageButtonGrid->addWidget(firstToolbarButton, 0, 0);
    imageButtonGrid->addWidget(secondToolbarButton, 0, 1);
    imageButtonGrid->addWidget(freezeSkillButton, 0, 2);
    imageButtonGrid->addWidget(shieldSkillButton, 1, 0);
    imageButtonGrid->addWidget(speedSkillButton, 1, 1);
    imageButtonGrid->addWidget(crystalLifeSaverButton, 1, 2);

    auto *battleSongLayout = new QHBoxLayout();
    battleSongLayout->setContentsMargins(0, 0, 0, 0);
    battleSongLayout->setSpacing(4);
    battleSongLayout->addWidget(battleSongButton);
    battleSongLayout->addWidget(secondBattleSongButton);

    panelLayout->addWidget(startButton);
    panelLayout->addWidget(resetButton);
    panelLayout->addLayout(battleSongLayout);
    panelLayout->addLayout(imageButtonGrid);
    panelLayout->addWidget(crystalShieldButton);
    panelLayout->addWidget(crystalShockwaveButton);
    panelLayout->addWidget(timeFreezeButton);

    m_messageLabel = new QLabel(QStringLiteral("选择塔并点击建塔点，准备守卫峡谷。"), panel);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setObjectName(QStringLiteral("messageLabel"));
    panelLayout->addWidget(m_messageLabel);
    panelLayout->addStretch(1);
    panelLayout->addWidget(crystalFailureButton, 0, Qt::AlignHCenter);

    rootLayout->addWidget(panel);
    m_stackLayout->addWidget(m_gamePage);
    m_videoOverlay = createVideoOverlay(central);
    m_stackLayout->addWidget(m_videoOverlay);
    m_videoOverlay->hide();
    setCentralWidget(central);

    setStyleSheet(QStringLiteral(R"(
        QMainWindow {
            background: #f4f1e8;
        }
        QLabel {
            color: #403b35;
            font-size: 15px;
        }
        QLabel#titleLabel {
            font-size: 21px;
            font-weight: 700;
            padding: 0 0 4px 0;
        }
        QLabel#messageLabel {
            background: #fff8df;
            border: 1px solid #d6c99d;
            border-radius: 6px;
            padding: 7px;
            line-height: 1.4;
        }
        QPushButton {
            background: #fffdf4;
            color: #403b35;
            border: 1px solid #c9bc96;
            border-radius: 6px;
            padding: 6px 8px;
            text-align: left;
            font-size: 14px;
        }
        QPushButton:hover {
            background: #fff3c2;
        }
        QPushButton:checked {
            background: #dcefcf;
            border: 2px solid #5f9c70;
            font-weight: 700;
        }
        QPushButton#startImageButton {
            background: transparent;
            border: 1px solid #c9bc96;
            border-radius: 6px;
            padding: 2px;
            text-align: center;
        }
        QPushButton#startImageButton:hover {
            background: #fff3c2;
        }
        QPushButton#sideImageButton {
            background: transparent;
            border: 1px solid #c9bc96;
            border-radius: 6px;
            padding: 2px;
            text-align: center;
        }
        QPushButton#sideImageButton:hover {
            background: #fff3c2;
        }
        QPushButton#skillImageButton {
            background: transparent;
            border: 1px solid #c9bc96;
            border-radius: 6px;
            padding: 2px;
            text-align: center;
        }
        QPushButton#skillImageButton:hover {
            background: #fff3c2;
        }
    )"));

    auto startCooldown = [](QPushButton *button, const QString &readyText, int cooldownSeconds) {
        button->setEnabled(false);
        auto *cooldownTimer = new QTimer(button);
        int *remainingSeconds = new int(cooldownSeconds);
        if (!readyText.isEmpty()) {
            button->setText(QStringLiteral("%1 (%2s)").arg(readyText).arg(*remainingSeconds));
        }
        QObject::connect(cooldownTimer, &QTimer::timeout, button, [button, cooldownTimer, remainingSeconds, readyText]() {
            --(*remainingSeconds);
            if (*remainingSeconds <= 0) {
                cooldownTimer->stop();
                cooldownTimer->deleteLater();
                delete remainingSeconds;
                button->setText(readyText);
                button->setEnabled(true);
                return;
            }
            if (!readyText.isEmpty()) {
                button->setText(QStringLiteral("%1 (%2s)").arg(readyText).arg(*remainingSeconds));
            }
        });
        cooldownTimer->start(1000);
    };

    connect(startButton, &QPushButton::clicked, this, &MainWindow::handleStartButtonClicked);
    // 按钮和场景之间通过 Qt 信号槽连接。
    connect(battleSongButton, &QPushButton::clicked, this, &MainWindow::playBattleSong);
    connect(secondBattleSongButton, &QPushButton::clicked, this, &MainWindow::playSecondBattleSong);
    connect(firstToolbarButton, &QPushButton::clicked, this, &MainWindow::playFirstToolbarSound);
    connect(secondToolbarButton, &QPushButton::clicked, this, &MainWindow::playSecondToolbarSound);
    connect(freezeSkillButton, &QPushButton::clicked, m_scene, &GameScene::freezeAllEnemies);
    connect(shieldSkillButton, &QPushButton::clicked, m_scene, &GameScene::shieldAllEnemies);
    connect(speedSkillButton, &QPushButton::clicked, m_scene, &GameScene::speedBoostAllEnemies);
    connect(crystalLifeSaverButton, &QPushButton::clicked, this, [this, crystalLifeSaverButton, startCooldown]() {
        if (m_scene->activateCrystalLifeSaver()) {
            startCooldown(crystalLifeSaverButton, QString(), 20);
        }
    });
    connect(crystalShieldButton, &QPushButton::clicked, this, [this, crystalShieldButton, startCooldown]() {
        m_scene->activateCrystalShield();
        startCooldown(crystalShieldButton, QStringLiteral("水晶护盾"), 20);
    });
    connect(crystalShockwaveButton, &QPushButton::clicked, this, [this, crystalShockwaveButton, startCooldown]() {
        m_scene->activateCrystalShockwave();
        startCooldown(crystalShockwaveButton, QStringLiteral("水晶冲击波"), 25);
    });
    connect(timeFreezeButton, &QPushButton::clicked, this, [this, timeFreezeButton, startCooldown]() {
        m_scene->activateTimeFreeze();
        startCooldown(timeFreezeButton, QStringLiteral("时间冻结"), 18);
    });
    connect(crystalFailureButton, &QPushButton::clicked, m_scene, &GameScene::triggerCrystalExplosionFailure);
    connect(resetButton, &QPushButton::clicked, m_scene, &GameScene::resetGame);
    connect(m_scene, &GameScene::statsChanged, this, &MainWindow::updateStats);
    connect(m_scene, &GameScene::messageChanged, m_messageLabel, &QLabel::setText);
    connect(m_scene, &GameScene::gameFinished, this, &MainWindow::showGameFinished);

    updateStats(m_scene->gold(), m_scene->lives(), m_scene->wave(), m_scene->maxWaves());
    selectTower(TowerType::Archer);
    playIntroVideo();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == m_videoOverlay || watched == m_videoWidget) && event->type() == QEvent::Resize) {
        updateIntroStartButtonGeometry();
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::updateStats(int gold, int lives, int wave, int maxWaves)
{
    // 游戏场景发出 statsChanged 后，这里刷新右侧状态文本。
    m_goldLabel->setText(QStringLiteral("金币：%1").arg(gold));
    m_livesLabel->setText(QStringLiteral("基地生命：%1").arg(lives));
    m_waveLabel->setText(QStringLiteral("波次：%1 / %2").arg(wave).arg(maxWaves));
}

void MainWindow::showGameFinished(bool victory, const QString &message)
{
    // 游戏结束时停止背景音乐，并播放胜利或失败视频。
    stopBackgroundMusic();
    m_messageLabel->setText(message);
    playResultVideo(victory);
    return;

    QMessageBox::information(this,
                             victory ? QStringLiteral("胜利") : QStringLiteral("失败"),
                             message);
}

void MainWindow::restartAfterResultVideo()
{
    hideResultVideo();
    if (m_scene) {
        m_scene->resetGame();
    }
}

void MainWindow::handleStartButtonClicked()
{
    // 开始新波次时，先播放按钮音效和背景音乐。
    playStartSound();
    startBackgroundMusic();
    if (m_scene) {
        m_scene->startNextWave();
    }
}

void MainWindow::enterGameFromIntro(bool startFirstWave)
{
    if (!m_introVideoMode && !m_videoOverlay->isVisible()) {
        return;
    }

    m_introVideoMode = false;
    if (m_introStartButton) {
        m_introStartButton->hide();
    }
    hideResultVideo();
    if (startFirstWave) {
        handleStartButtonClicked();
    }
}

void MainWindow::updateIntroStartButtonGeometry()
{
    if (!m_introStartButton || !m_videoWidget) {
        return;
    }

    const QRect contentRect = introVideoContentRect(m_videoWidget->geometry());
    const int buttonWidth = qRound(contentRect.width() * 0.30);
    const int buttonHeight = qRound(contentRect.height() * 0.14);
    const int buttonX = contentRect.left() + qRound(contentRect.width() * 0.35);
    const int buttonY = contentRect.top() + qRound(contentRect.height() * 0.68);
    m_introStartButton->setGeometry(buttonX, buttonY, buttonWidth, buttonHeight);
    if (m_introVideoMode) {
        m_introStartButton->raise();
    }
}

QPushButton *MainWindow::createTowerButton(TowerType type)
{
    // 根据炮塔类型生成右侧的建塔按钮。
    const int cost = Tower::buildCost(type);
    QString detail;
    switch (type) {
    case TowerType::Archer:
        detail = QStringLiteral("攻速快，单体输出");
        break;
    case TowerType::Mage:
        detail = QStringLiteral("伤害高，攻速较慢");
        break;
    case TowerType::Support:
        detail = QStringLiteral("伤害低，附带减速");
        break;
    }

    QString iconPath;
    switch (type) {
    case TowerType::Archer:
        iconPath = QStringLiteral(R"(D:\cpp\picture\2444f6c9f51e8cccfcdba7c49743d10b.jpg)");
        break;
    case TowerType::Mage:
        iconPath = QStringLiteral(R"(D:\cpp\picture\a71292cbbbf50eb54901cad379ae7e24.jpg)");
        break;
    case TowerType::Support:
        iconPath = QStringLiteral(R"(D:\cpp\picture\0eaa49960c1fbdccd29835e2b19eb94a.jpg)");
        break;
    }

    auto *button = new QPushButton(QStringLiteral("%1  %2 金币\n%3")
                                       .arg(Tower::typeName(type))
                                       .arg(cost)
                                       .arg(detail));
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(40, 40));
    button->setCheckable(true);
    button->setMinimumHeight(64);
    return button;
}

void MainWindow::selectTower(TowerType type)
{
    if (m_scene) {
        m_scene->setSelectedTowerType(type);
    }
}

QWidget *MainWindow::createVideoOverlay(QWidget *parent)
{
    // 覆盖在游戏页面上的全屏视频层，用于开场和结算。
    auto *overlay = new QWidget(parent);
    overlay->setObjectName(QStringLiteral("resultOverlay"));
    overlay->setAutoFillBackground(true);
    overlay->setStyleSheet(QStringLiteral(R"(
        QWidget#resultOverlay {
            background: #050505;
        }
        QWidget#resultButtonPanel {
            background: rgba(0, 0, 0, 180);
        }
        QLabel#resultStatsLabel {
            color: #fff8df;
            font-size: 18px;
            font-weight: 700;
            padding: 8px 18px;
        }
        QPushButton#resultButton {
            background: #fffdf4;
            color: #302a24;
            border: 1px solid #c9bc96;
            border-radius: 6px;
            padding: 10px 18px;
            text-align: center;
            font-size: 16px;
            font-weight: 700;
            min-width: 120px;
        }
        QPushButton#resultButton:hover {
            background: #fff3c2;
        }
    )"));

    auto *layout = new QVBoxLayout(overlay);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_videoWidget = new QVideoWidget(overlay);
    m_videoWidget->setMinimumSize(640, 360);
    m_videoWidget->setAspectRatioMode(Qt::KeepAspectRatio);
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(m_videoWidget, 1);

    m_introStartButton = new QPushButton(overlay);
    m_introStartButton->setObjectName(QStringLiteral("introStartButton"));
    m_introStartButton->setCursor(Qt::PointingHandCursor);
    m_introStartButton->setFocusPolicy(Qt::NoFocus);
    m_introStartButton->setFlat(true);
    m_introStartButton->setStyleSheet(QStringLiteral(R"(
        QPushButton#introStartButton {
            background: transparent;
            border: none;
        }
    )"));
    m_introStartButton->hide();

    m_resultButtonPanel = new QWidget(overlay);
    m_resultButtonPanel->setObjectName(QStringLiteral("resultButtonPanel"));
    auto *buttonLayout = new QHBoxLayout(m_resultButtonPanel);
    buttonLayout->setContentsMargins(18, 14, 18, 14);
    buttonLayout->setSpacing(16);

    m_resultStatsLabel = new QLabel(m_resultButtonPanel);
    m_resultStatsLabel->setObjectName(QStringLiteral("resultStatsLabel"));
    m_resultStatsLabel->setWordWrap(true);
    buttonLayout->addWidget(m_resultStatsLabel, 1);
    buttonLayout->addStretch(1);

    auto *restartButton = new QPushButton(QStringLiteral("重新开始"), m_resultButtonPanel);
    restartButton->setObjectName(QStringLiteral("resultButton"));
    auto *quitButton = new QPushButton(QStringLiteral("退出游戏"), m_resultButtonPanel);
    quitButton->setObjectName(QStringLiteral("resultButton"));
    buttonLayout->addWidget(restartButton);
    buttonLayout->addWidget(quitButton);
    buttonLayout->addStretch(1);
    layout->addWidget(m_resultButtonPanel);
    m_resultButtonPanel->hide();

    m_videoPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(1.0);
    m_videoPlayer->setVideoOutput(m_videoWidget);
    m_videoPlayer->setAudioOutput(m_audioOutput);

    connect(m_videoPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        qDebug() << "Main video media status:" << status;
        if (status == QMediaPlayer::EndOfMedia || status == QMediaPlayer::InvalidMedia) {
            if (m_resultVideoMode) {
                showResultButtons();
            } else if (m_introVideoMode) {
                enterGameFromIntro(false);
            } else {
                hideResultVideo();
            }
        }
    });
    connect(m_videoPlayer, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &errorString) {
        qDebug() << "Main video error:" << errorString;
        if (m_resultVideoMode) {
            m_messageLabel->setText(QStringLiteral("结果视频播放失败：%1").arg(errorString));
            showResultButtons();
        } else if (m_introVideoMode) {
            m_messageLabel->setText(QStringLiteral("开场视频播放失败：%1").arg(errorString));
            enterGameFromIntro(false);
        } else {
            hideResultVideo();
        }
    });
    connect(m_introStartButton, &QPushButton::clicked, this, [this]() {
        enterGameFromIntro(false);
    });
    connect(restartButton, &QPushButton::clicked, this, &MainWindow::restartAfterResultVideo);
    connect(quitButton, &QPushButton::clicked, this, &QWidget::close);

    overlay->installEventFilter(this);
    m_videoWidget->installEventFilter(this);

    return overlay;
}

void MainWindow::playIntroVideo()
{
    if (!m_videoPlayer || !m_videoOverlay || !m_stackLayout) {
        return;
    }

    // 程序启动后播放开场视频；文件不存在时直接回到游戏页面。
    const QString introPath = QStringLiteral(R"(D:\cpp\picture\1c34b3acec1a5b6fd8f36eaef86d0149.mp4)");
    if (!QFileInfo::exists(introPath)) {
        qDebug() << "Intro video file does not exist:" << introPath;
        hideResultVideo();
        return;
    }

    m_resultVideoMode = false;
    m_introVideoMode = true;
    m_resultButtonPanel->hide();
    m_videoOverlay->show();
    m_videoOverlay->raise();
    m_stackLayout->setCurrentWidget(m_videoOverlay);
    if (m_videoWidget) {
        m_videoWidget->setAspectRatioMode(Qt::KeepAspectRatio);
        m_videoWidget->show();
    }
    if (m_introStartButton) {
        updateIntroStartButtonGeometry();
        m_introStartButton->show();
        m_introStartButton->raise();
    }
    m_videoPlayer->stop();
    m_videoPlayer->setVideoOutput(m_videoWidget);
    m_videoPlayer->setAudioOutput(m_audioOutput);
    m_videoPlayer->setSource(QUrl::fromLocalFile(introPath));
    m_videoPlayer->play();
}

void MainWindow::playResultVideo(bool victory)
{
    if (!m_videoPlayer || !m_videoOverlay || !m_stackLayout) {
        return;
    }

    // 根据胜负选择不同的结算视频。
    m_resultVideoMode = true;
    m_introVideoMode = false;
    if (m_introStartButton) {
        m_introStartButton->hide();
    }
    m_resultButtonPanel->hide();
    m_videoOverlay->show();
    m_videoOverlay->raise();
    m_stackLayout->setCurrentWidget(m_videoOverlay);
    m_videoPlayer->stop();
    if (victory) {
        m_videoPlayer->setSource(QUrl::fromLocalFile(R"(D:\cpp\picture\f580f2721dfe41996973150708cf63a0.mp4)"));
    } else {
        m_videoPlayer->setSource(QUrl::fromLocalFile(R"(D:\cpp\picture\456318d387208ef4f627ce975f92a8ff.mp4)"));
    }
    m_videoPlayer->play();
}

void MainWindow::playStartSound()
{
    if (!m_startSoundPlayer) {
        m_startSoundPlayer = new QMediaPlayer(this);
        m_startAudioOutput = new QAudioOutput(this);
        m_startAudioOutput->setVolume(0.9);
        m_startSoundPlayer->setAudioOutput(m_startAudioOutput);
        m_startSoundPlayer->setSource(QUrl::fromLocalFile(R"(D:\cpp\picture\15ef4d96343e429c88e4bb8c21bea2ae.mp4)"));
    }

    if (m_startSoundPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_startSoundPlayer->stop();
    }
    m_startSoundPlayer->setPosition(0);
    m_startSoundPlayer->play();
}

void MainWindow::playBattleSong()
{
    if (!m_battleSongPlayer) {
        m_battleSongPlayer = new QMediaPlayer(this);
        m_battleSongAudioOutput = new QAudioOutput(this);
        m_battleSongAudioOutput->setVolume(0.9);
        m_battleSongPlayer->setAudioOutput(m_battleSongAudioOutput);
        m_battleSongPlayer->setSource(QUrl::fromLocalFile(R"(D:\cpp\picture\5462aaeecf02d9a107f6a53aa37488c3.mp4)"));
    }

    if (m_battleSongPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_battleSongPlayer->stop();
    }
    m_battleSongPlayer->setPosition(0);
    m_battleSongPlayer->play();
}

void MainWindow::playSecondBattleSong()
{
    if (!m_secondBattleSongPlayer) {
        m_secondBattleSongPlayer = new QMediaPlayer(this);
        m_secondBattleSongAudioOutput = new QAudioOutput(this);
        m_secondBattleSongAudioOutput->setVolume(0.9);
        m_secondBattleSongPlayer->setAudioOutput(m_secondBattleSongAudioOutput);
        m_secondBattleSongPlayer->setSource(QUrl::fromLocalFile(R"(D:\cpp\picture\30ac133983d6d134ccfabc31350e10a8.mp4)"));
    }

    if (m_secondBattleSongPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_secondBattleSongPlayer->stop();
    }
    m_secondBattleSongPlayer->setPosition(0);
    m_secondBattleSongPlayer->play();
}

void MainWindow::playFirstToolbarSound()
{
    if (!m_firstToolbarPlayer) {
        m_firstToolbarPlayer = new QMediaPlayer(this);
        m_firstToolbarAudioOutput = new QAudioOutput(this);
        m_firstToolbarAudioOutput->setVolume(0.9);
        m_firstToolbarPlayer->setAudioOutput(m_firstToolbarAudioOutput);
        m_firstToolbarPlayer->setSource(QUrl::fromLocalFile(R"(D:\cpp\picture\1d9036df53b0c1c0fe174308fd9daa1e.mp4)"));
    }

    if (m_firstToolbarPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_firstToolbarPlayer->stop();
    }
    m_firstToolbarPlayer->setPosition(0);
    m_firstToolbarPlayer->play();
}

void MainWindow::playSecondToolbarSound()
{
    if (!m_secondToolbarPlayer) {
        m_secondToolbarPlayer = new QMediaPlayer(this);
        m_secondToolbarAudioOutput = new QAudioOutput(this);
        m_secondToolbarAudioOutput->setVolume(0.9);
        m_secondToolbarPlayer->setAudioOutput(m_secondToolbarAudioOutput);
        m_secondToolbarPlayer->setSource(QUrl::fromLocalFile(R"(D:\cpp\picture\5289b75aea6175696862abc3c7c5bb28.mp4)"));
    }

    if (m_secondToolbarPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_secondToolbarPlayer->stop();
    }
    m_secondToolbarPlayer->setPosition(0);
    m_secondToolbarPlayer->play();
}

void MainWindow::startBackgroundMusic()
{
    // 背景音乐设置为无限循环，只在第一次调用时创建播放器。
    if (!m_backgroundMusicPlayer) {
        m_backgroundMusicPlayer = new QMediaPlayer(this);
        m_backgroundAudioOutput = new QAudioOutput(this);
        m_backgroundAudioOutput->setVolume(0.45);
        m_backgroundMusicPlayer->setAudioOutput(m_backgroundAudioOutput);
        m_backgroundMusicPlayer->setLoops(QMediaPlayer::Infinite);
        m_backgroundMusicPlayer->setSource(QUrl::fromLocalFile(R"(D:\cpp\picture\05b585f45863e927c4b23479727465bf.mp4)"));
    }

    if (m_backgroundMusicPlayer->playbackState() != QMediaPlayer::PlayingState) {
        m_backgroundMusicPlayer->play();
    }
}

void MainWindow::stopBackgroundMusic()
{
    if (m_backgroundMusicPlayer) {
        m_backgroundMusicPlayer->stop();
    }
}

void MainWindow::showResultButtons()
{
    if (m_resultStatsLabel && m_scene) {
        m_resultStatsLabel->setText(QStringLiteral("结算统计\n总击杀数：%1    漏掉的小兵：%2    剩余金币：%3\n已通关波数：%4 / %5    最高防御塔等级：%6    使用技能次数：%7")
                                        .arg(m_scene->killCount())
                                        .arg(m_scene->leakedCount())
                                        .arg(m_scene->gold())
                                        .arg(m_scene->completedWaveCount())
                                        .arg(m_scene->maxWaves())
                                        .arg(m_scene->highestTowerLevel())
                                        .arg(m_scene->skillUseCount()));
    }
    if (m_resultButtonPanel) {
        m_resultButtonPanel->show();
    }
}

void MainWindow::hideResultVideo()
{
    // 关闭视频层并切回游戏页面。
    m_introVideoMode = false;
    if (m_videoPlayer) {
        m_videoPlayer->stop();
    }
    if (m_introStartButton) {
        m_introStartButton->hide();
    }
    if (m_resultButtonPanel) {
        m_resultButtonPanel->hide();
    }
    if (m_videoOverlay) {
        m_videoOverlay->hide();
    }
    if (m_stackLayout && m_gamePage) {
        m_stackLayout->setCurrentWidget(m_gamePage);
    }
}

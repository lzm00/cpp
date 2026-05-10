#include "MainWindow.h"

#include <QApplication>

// 程序入口：创建 Qt 应用对象，显示主窗口，然后进入事件循环。
int main(int argc, char *argv[])
{
    // 讲解点：main.cpp 是程序入口，负责创建 Qt 应用对象并启动主窗口。
    QApplication app(argc, argv);

    // 讲解点：MainWindow 搭建游戏界面，并间接持有核心游戏场景 GameScene。
    MainWindow window;
    window.show();

    // 讲解点：app.exec() 进入 Qt 事件循环，之后按钮点击、定时器刷新都会由事件循环驱动。
    return app.exec();
}

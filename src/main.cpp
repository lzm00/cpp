#include "MainWindow.h"

#include <QApplication>

// 程序入口：创建 Qt 应用对象，显示主窗口，然后进入事件循环。
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}

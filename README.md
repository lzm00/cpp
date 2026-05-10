# Valley Guardian TD

一个基于 Qt Widgets 和 C++17 的简易塔防游戏项目。工程按照提纲实现了地图路径、建塔点、敌人波次、防御塔、子弹、金币、生命值、胜负判定和基础 UI。

## 打开方式

推荐使用 Qt Creator：

1. 打开 `ValleyGuardianTD.pro`，或打开 `CMakeLists.txt`。
2. 选择一个包含 Qt Widgets 的 Kit。
3. 构建并运行。

如果命令行已经配置好 Qt 和 CMake：

```powershell
cmake -S . -B build
cmake --build build
```

## 地图背景

程序会通过 `resources.qrc` 将 `assets/valley_map.png` 编译进程序，游戏启动时优先从 Qt Resource 加载背景图。

替换地图时，把新图覆盖到 `assets/valley_map.png`，然后在 Qt Creator 中执行一次“运行 qmake”或重新配置 CMake，再重新构建。

当前地图逻辑已经按标注图调整：

- 图中的防御塔位置作为可建造/升级的攻击塔位。
- 红色圈出的两个位置作为小兵刷新点。
- 红色箭头方向作为小兵行进路线。
- 小兵最终攻打右上角红色水晶。

## 玩法

- 点击右侧塔类型按钮选择塔。
- 点击地图上的空圆形建塔点建造防御塔。
- 点击已有防御塔升级。
- 点击“开始波次”生成敌人。
- 消灭敌人获得金币，所有波次清空即胜利，生命值归零即失败。

## 塔类型

- 射手塔：攻速快，单体伤害稳定。
- 法师塔：伤害高，攻速慢。
- 辅助塔：伤害较低，可以减速敌人。

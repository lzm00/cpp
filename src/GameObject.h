#pragma once

#include <QtGlobal>

// 所有需要“每帧更新”的游戏对象都实现这个接口。
// 讲解点：GameObject 是统一的更新接口，Enemy、Tower、Projectile 都通过 updateObject() 参与每帧刷新。
class GameObject
{
public:
    virtual ~GameObject() = default;
    // dt 表示距离上一帧过去的时间，单位是秒。
    virtual void updateObject(qreal dt) = 0;
};

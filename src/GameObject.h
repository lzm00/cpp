#pragma once

#include <QtGlobal>

class GameObject
{
public:
    virtual ~GameObject() = default;
    virtual void updateObject(qreal dt) = 0;
};

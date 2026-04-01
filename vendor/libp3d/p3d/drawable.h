// drawable.h — tDrawable base class (Pure3D v11.3)
#pragma once

#include "p3d/entity.h"

class tDrawable : public tEntity {
public:
    virtual void Display() = 0;

protected:
    virtual ~tDrawable() = default;
};

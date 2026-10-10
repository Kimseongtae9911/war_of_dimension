#pragma once
#include <ServerCore/GameObject.h>

namespace wod_server
{
class CTransform : public wod::core::ObjectPosition<vec3, vec2>, public wod::core::ObjectOrientation<vec3>
{
  public:
    CTransform();
    void Reset();
    void Move(const vec3& _shift, float _height);
    bool CheckDistance(const vec3& _pos);
};
}

#pragma once
#include <ServerCore/GameObject.h>

namespace wod_server
{
class CPhysic : public wod::core::ObjectMotion<vec3>
{
  public:
    CPhysic();

    vec3& GetVelocity()
    {
        return m_vel;
    }

    const vec3& CalculateMoveShift(char _dir, const vec3& _look, const vec3& _right);
    void Deceleration(float _elapsedTime);
};
}

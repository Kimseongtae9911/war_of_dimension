#include "pch.h"
#include "CPhysic.h"

namespace wod_server
{
CPhysic::CPhysic()
{
    m_maxVelXZ = PLAYER_MAX_VELXZ + 4.0f;
    m_maxVelY = PLAYER_MAX_VELY;
    m_friction = WORLD_FRICTION;

    m_vel = {0.f, 0.f, 0.f};
}

const vec3& CPhysic::CalculateMoveShift(char _dir, const vec3& _look, const vec3& _right)
{
    vec3 shift = {0, 0, 0};

    if (_dir & DIR_FORWARD)
    {
        shift = vec3::Add(shift, _look, PLAYER_SPEED);
    }

    if (_dir & DIR_BACKWARD)
    {
        shift = vec3::Add(shift, _look, -PLAYER_SPEED);
    }

    if (_dir & DIR_RIGHT)
    {
        shift = vec3::Add(shift, _right, PLAYER_SPEED);
    }

    if (_dir & DIR_LEFT)
    {
        shift = vec3::Add(shift, _right, -PLAYER_SPEED);
    }

    // 방향 입력을 목표 속도로 해석한다. 갱신 횟수와 대각 입력에 속도가 의존하지 않는다.
    m_vel = vec3::Normalize(shift) * m_maxVelXZ;

    return m_vel;
}

}

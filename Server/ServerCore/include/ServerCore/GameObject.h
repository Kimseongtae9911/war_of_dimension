#pragma once

namespace wod::core
{
// 벡터·충돌 타입은 서버가 주입한다. Core는 DirectX와 게임 규칙에 의존하지 않는다.
template <class Vector3, class Vector2> class ObjectPosition
{
  public:
    const Vector3& GetPos() const
    {
        return m_pos;
    }

    void SetPos(float _x, float _y, float _z)
    {
        m_pos.m_x = _x;
        m_pos.m_y = _y;
        m_pos.m_z = _z;
    }

    void SetPos(const Vector3& _pos)
    {
        m_pos = _pos;
    }

    void SetPos(const Vector2& _pos)
    {
        m_pos.m_x = _pos.m_x;
        m_pos.m_z = _pos.m_z;
    }

  protected:
    Vector3 m_pos{};
};

template <class Vector3> class ObjectOrientation
{
  public:
    char GetDir() const
    {
        return m_dir;
    }

    void SetDir(char _dir)
    {
        m_dir = _dir;
    }

    const Vector3& GetLook() const
    {
        return m_look;
    }

    const Vector3& GetUp() const
    {
        return m_up;
    }

    const Vector3& GetRight() const
    {
        return m_right;
    }

    void SetLook(const Vector3& _look)
    {
        m_look = _look;
    }

    void SetRight(const Vector3& _right)
    {
        m_right = _right;
    }

  protected:
    char m_dir = 0;
    Vector3 m_look{};
    Vector3 m_up{};
    Vector3 m_right{};
};

template <class Vector3> class ObjectMotion
{
  public:
    const Vector3& GetVelocity() const
    {
        return m_vel;
    }

    void SetVelocity(const Vector3& _velocity)
    {
        m_vel = _velocity;
    }

  protected:
    Vector3 m_vel{};
    float m_maxVelXZ = 0.0f;
    float m_maxVelY = 0.0f;
    float m_friction = 0.0f;
};

template <class Geometry> class GameObject : public ObjectPosition<typename Geometry::Vector3, typename Geometry::Vector2>
{
  public:
    virtual ~GameObject() = default;

    int GetID() const
    {
        return m_id;
    }

    void SetID(int _id)
    {
        m_id = _id;
    }

    virtual bool Update([[maybe_unused]] float _elapsedTime)
    {
        return true;
    }

    const typename Geometry::BoundingBox& GetBoundingBox() const
    {
        return m_boundingBox;
    }

    const typename Geometry::Matrix& GetWorldMatrix() const
    {
        return m_worldMatrix;
    }

  protected:
    int m_id = -1;
    typename Geometry::BoundingBox m_boundingBox;
    typename Geometry::BoundingBox m_initBoundingBox;
    typename Geometry::Matrix m_worldMatrix;
};

template <class Geometry> class MoveObject : public GameObject<Geometry>, public ObjectOrientation<typename Geometry::Vector3>, public ObjectMotion<typename Geometry::Vector3>
{
  public:
    using Vector3 = typename Geometry::Vector3;

    void SetLook(const Vector3& _look)
    {
        this->m_look = _look;
        this->m_right = Vector3::Normalize(this->m_up.Cross(this->m_look));
    }

    int GetMatchNum() const
    {
        return m_matchNum;
    }

    int GetCurNode() const
    {
        return m_curNode;
    }

    void SetMatchNum(int _number)
    {
        m_matchNum = _number;
    }

    void SetCurNode(int _node)
    {
        m_curNode = _node;
    }

    virtual void Move([[maybe_unused]] float _elapsedTime)
    {
    }

    virtual void UpdateBoundingBox()
    {
        Geometry::UpdateBoundingBox(this->m_right, this->m_look, this->m_pos, this->m_worldMatrix, this->m_initBoundingBox, this->m_boundingBox);
    }

  protected:
    int m_matchNum = -1;
    int m_curNode = -1;
};
}

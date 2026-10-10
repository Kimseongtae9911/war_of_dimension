#include <ServerCore/GameObject.h>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace
{
struct Vector2
{
    float m_x, m_z;
};

struct Vector3
{
    float m_x = 0, m_y = 0, m_z = 0;

    Vector3 Cross(const Vector3& _other) const
    {
        return {m_y * _other.m_z - m_z * _other.m_y, m_z * _other.m_x - m_x * _other.m_z, m_x * _other.m_y - m_y * _other.m_x};
    }

    static Vector3 Normalize(const Vector3& _vector)
    {
        const auto length = std::sqrt(_vector.m_x * _vector.m_x + _vector.m_y * _vector.m_y + _vector.m_z * _vector.m_z);

        return {_vector.m_x / length, _vector.m_y / length, _vector.m_z / length};
    }

    bool operator==(const Vector3&) const = default;
};

struct Geometry
{
    using Vector3 = ::Vector3;
    using Vector2 = ::Vector2;

    struct BoundingBox
    {
        int m_value = 0;
    };

    struct Matrix
    {
        int m_untouched = 27;
        Vector3 m_right, m_look, m_pos;
    };

    static void UpdateBoundingBox(const Vector3& _right, const Vector3& _look, const Vector3& _pos, Matrix& _world, const BoundingBox& _initial, BoundingBox& _result)
    {
        _world.m_right = _right;
        _world.m_look = _look;
        _world.m_pos = _pos;
        _result.m_value = _initial.m_value + 1;
    }
};

struct Moving : wod::core::MoveObject<Geometry>
{
    Moving()
    {
        m_up = {0, 1, 0};
        m_initBoundingBox.m_value = 41;
    }
};

struct Overridden : Moving
{
    explicit Overridden(bool& _destroyed) : m_destroyed(_destroyed)
    {
    }

    ~Overridden() override
    {
        m_destroyed = true;
    }

    bool Update(float _elapsedTime) override
    {
        m_pos.m_y += _elapsedTime;

        return false;
    }

    void Move(float _elapsedTime) override
    {
        m_pos.m_x += _elapsedTime;
    }

    void UpdateBoundingBox() override
    {
        m_boundingBox.m_value = 99;
    }

    bool& m_destroyed;
};

void Check(bool _condition, const char *_message, int& _checks)
{
    ++_checks;
    if (!_condition)
        throw std::runtime_error(_message);
}
}

int RunGameObjectTests()
{
    int checks = 0;
    Moving object;
    static_assert(std::is_same_v<decltype(object.GetVelocity()), const Vector3&>);
    Check(object.GetID() == -1 && object.GetMatchNum() == -1 && object.GetCurNode() == -1, "object default identity", checks);
    object.SetID(7);
    object.SetMatchNum(2);
    object.SetCurNode(3);
    Check(object.GetID() == 7 && object.GetMatchNum() == 2 && object.GetCurNode() == 3, "object identity and navigation state", checks);
    object.SetPos(1, 8, 3);
    object.SetPos(Vector2{5, 6});
    Check(object.GetPos() == Vector3{5, 8, 6}, "2d position retains height", checks);
    object.SetPos(Vector3{2, 4, 6});
    Check(object.GetPos() == Vector3{2, 4, 6}, "3d position replaces all axes", checks);
    object.SetDir(5);
    object.SetLook({0, 0, 2});
    Check(object.GetDir() == 5 && object.GetLook() == Vector3{0, 0, 2} && object.GetRight() == Vector3{1, 0, 0}, "game look recomputes normalized right", checks);
    wod::core::ObjectOrientation<Vector3> lobbyOrientation;
    lobbyOrientation.SetRight({9, 8, 7});
    lobbyOrientation.SetLook({1, 0, 0});
    Check(lobbyOrientation.GetLook() == Vector3{1, 0, 0} && lobbyOrientation.GetRight() == Vector3{9, 8, 7}, "lobby look retains explicit right", checks);
    object.SetVelocity({3, 0, 4});
    Check(object.GetVelocity() == Vector3{3, 0, 4}, "shared velocity state", checks);
    Moving other;
    Check(other.GetVelocity() == Vector3{} && other.GetPos() == Vector3{}, "object state independent", checks);
    object.UpdateBoundingBox();
    Check(object.GetWorldMatrix().m_right == object.GetRight() && object.GetWorldMatrix().m_look == object.GetLook() && object.GetWorldMatrix().m_pos == object.GetPos(), "geometry receives current orientation and position", checks);
    Check(object.GetBoundingBox().m_value == 42 && object.GetWorldMatrix().m_untouched == 27, "geometry retains initial bounds and unmodified matrix fields", checks);
    Check(object.Update(0), "default object update", checks);
    bool destroyed = false;
    {
        std::unique_ptr<wod::core::MoveObject<Geometry>> derived = std::make_unique<Overridden>(destroyed);
        derived->Move(3);
        Check(!derived->Update(2) && derived->GetPos() == Vector3{3, 2, 0}, "move and update virtual dispatch", checks);
        derived->UpdateBoundingBox();
        Check(derived->GetBoundingBox().m_value == 99, "bounds virtual dispatch", checks);
    }
    Check(destroyed, "object polymorphic destruction", checks);

    return checks;
}

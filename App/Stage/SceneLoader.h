#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include <World3D/Collider/ColliderStructs.h>
#include <Math/MathUtils.h>

struct EnemySpawnInfo {
    std::string name;
    float delay;
};

struct EventCondition {
    std::string type;
    std::vector<std::string> targets;
};

struct EventInfo {
    std::string type;
    std::string trigger;
    std::vector<EnemySpawnInfo> enemies;    // EnemySpawn 用
    std::vector<EventCondition> conditions; // ClearEvent 用
};

enum class ColliderType {
    None,
    AABB,
    Sphere,
};

struct Collider {
    ColliderType type = ColliderType::None;
    AABBData aabb;
    SphereData sphere;
};

struct SceneObject {
    std::string name;
    std::string className    = "Object3d";
    EulerTransform transform;
    std::optional<std::string> fileName;
    std::optional<Collider> collider;
    std::optional<EventInfo> eventInfo;
};


class SceneLoader
{
public:
    static std::vector<SceneObject> Load(const std::string& path);

private:
    static void ParseObject(const nlohmann::json& j, SceneObject& out);
    static Collider ParseCollider(const nlohmann::json& j);
};

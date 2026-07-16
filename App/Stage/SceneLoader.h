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
    std::string bossName;                   // BossSpawn 用（出現させるボスのオブジェクト名）
};

// レベルエディタで配置したポイントライトの情報（PointLight / ライト付きProp 用）
// offset は Blender ローカル座標のまま保持する（Y/Z の入れ替えは SceneBuilder が行う）
struct LightInfo {
    Vector3 color = { 1.0f, 1.0f, 1.0f };
    Vector3 offset = { 0.0f, 0.0f, 0.0f };
    float intensity = 1.5f;
    float radius = 10.0f;
    float decay = 1.0f;
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
    std::optional<LightInfo> lightInfo;
};


class SceneLoader
{
public:
    static std::vector<SceneObject> Load(const std::string& path);

private:
    static void ParseObject(const nlohmann::json& j, SceneObject& out);
    static Collider ParseCollider(const nlohmann::json& j);
};

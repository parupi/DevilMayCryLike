#pragma once
#include <fstream>
#include <nlohmann/json.hpp> 
#include <3d/Collider/ColliderStructs.h>
#include <math/function.h>

using json = nlohmann::json;

enum class ColliderType {
    None,
    AABB,
    Sphere,
};

struct Collider {
    ColliderType type = ColliderType::None;

    // AABB用
    AABBData aabb;

    // Sphere用
    SphereData sphere;
};

struct SceneObject {
    std::string name_;
    std::string type;
    std::string className = "Object3d"; // 追加
    EulerTransform transform;
    std::optional<std::string> fileName;
    std::optional<Collider> collider;
    std::vector<SceneObject> children;
};


class SceneLoader
{
public:
    // JSONファイルからルートのSceneObject群を読み込む
    static std::vector<SceneObject> Load(const std::string& path);

private:
    static void ParseObject(const nlohmann::json& json, SceneObject& outObject);
    static Collider ParseCollider(const nlohmann::json& json);
};


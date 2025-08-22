#include "SceneLoader.h"
#include <fstream>

using json = nlohmann::json;

std::vector<SceneObject> SceneLoader::Load(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open JSON file: " + path);
    }

    json root;
    file >> root;

    std::vector<SceneObject> objects;
    for (const auto& obj : root["objects"]) {
        SceneObject sceneObj;
        ParseObject(obj, sceneObj);
        objects.push_back(sceneObj);
    }

    return objects;
}

void SceneLoader::ParseObject(const json& objJson, SceneObject& outObject) {
    outObject.name_ = objJson["name"];
    outObject.className = objJson.value("class", "Object3d");

    const auto& transform = objJson["transform"];
    outObject.transform.translate = {
        transform["translation"][0],
        transform["translation"][1],
        transform["translation"][2],
    };
    outObject.transform.rotate = {
        transform["rotation"][0],
        transform["rotation"][1],
        transform["rotation"][2],
    };
    outObject.transform.scale = {
        transform["scaling"][0],
        transform["scaling"][1],
        transform["scaling"][2],
    };

    if (objJson.contains("file_name")) {
        outObject.fileName = objJson["file_name"];
    }

    if (objJson.contains("collider")) {
        outObject.collider = ParseCollider(objJson["collider"]);
    }

    if (objJson.contains("event")) {
        EventInfo eventInfo;
        const auto& eventJson = objJson["event"];

        eventInfo.type = eventJson.value("type", "");
        eventInfo.trigger = eventJson.value("trigger", "");

        if (eventJson.contains("enemies")) {
            for (const auto& enemyJson : eventJson["enemies"]) {
                EnemySpawnInfo enemyInfo;
                enemyInfo.name = enemyJson["name"];
                enemyInfo.delay = enemyJson.value("delay", 0.0f);
                eventInfo.enemies.push_back(enemyInfo);
            }
        }

        outObject.eventInfo = eventInfo;
    }

    if (objJson.contains("children")) {
        for (const auto& childJson : objJson["children"]) {
            SceneObject child;
            ParseObject(childJson, child);
            outObject.children.push_back(child);
        }
    }
}

Collider SceneLoader::ParseCollider(const json& colliderJson) {
    Collider collider;

    std::string type = colliderJson["type"];
    if (type == "BOX") {
        collider.type = ColliderType::AABB;

        Vector3 center = {
            colliderJson["center"][0],
            colliderJson["center"][1],
            colliderJson["center"][2],
        };
        Vector3 size = {
            colliderJson["size"][0],
            colliderJson["size"][1],
            colliderJson["size"][2],
        };

        collider.aabb.offsetMin = {
            center.x - size.x * 0.5f,
            center.y - size.y * 0.5f,
            center.z - size.z * 0.5f,
        };
        collider.aabb.offsetMax = {
            center.x + size.x * 0.5f,
            center.y + size.y * 0.5f,
            center.z + size.z * 0.5f,
        };
    } else if (type == "SPHERE") {
        collider.type = ColliderType::Sphere;
        collider.sphere.offset = {
            colliderJson["center"][0],
            colliderJson["center"][1],
            colliderJson["center"][2],
        };
        collider.sphere.radius = colliderJson["size"][0] * 0.5f;
    }

    return collider;
}
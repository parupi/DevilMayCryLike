#pragma once

#include <vector>
#include <Include/SceneLoader.h>

class Object3d;
class Camera;

class SceneBuilder {
public:
    // ルートノード群からシーン構築
    static void BuildScene(const std::vector<SceneObject>& sceneObjects);

private:
    static std::unique_ptr<Object3d> BuildObjectRecursive(const SceneObject& sceneObj, Object3d* parent);

    static std::unique_ptr<Object3d> CreateEvents(const SceneObject& sceneObj);

    // 最後に生成するイベントリスト
    static std::vector<const SceneObject*> pendingEvents_;
    static void BuildPendingEvents();

};

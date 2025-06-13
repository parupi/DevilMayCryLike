#pragma once

#include <vector>
#include <Include/SceneLoader.h>

class Object3d;
class Camera;

class SceneBuilder {
public:
    // ルートノード群からシーン構築
    static std::vector<Object3d*> BuildScene(const std::vector<SceneObject>& sceneObjects);

private:
    static Object3d* BuildObjectRecursive(const SceneObject& sceneObj, Object3d* parent);
};

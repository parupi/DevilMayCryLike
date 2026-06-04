#pragma once
#include <vector>
#include <Stage/SceneLoader.h>

class Object3d;
class WorldTransform;

class SceneBuilder {
public:
	static void BuildScene(const std::vector<SceneObject>& sceneObjects);

private:
	static bool IsEvent(const SceneObject& obj);

	// エディタとエンジンの座標系変換を適用 (Y↔Z スワップ)
	static void ApplyTransform(WorldTransform* transform, const EulerTransform& src);

	// コライダーを生成してオブジェクトに登録
	static void ApplyCollider(Object3d* object, const std::string& name, const Collider& col);

	// 通常オブジェクトを生成してマネージャーに登録
	static void BuildObject(const SceneObject& sceneObj, std::vector<SceneObject>& outPendingEvents);

	// イベントオブジェクトを生成してマネージャーに登録
	static void BuildEvent(const SceneObject& sceneObj);
};

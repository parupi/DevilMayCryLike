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

	// ステージデータはエンジン空間なのでそのまま流し込む
	static void ApplyTransform(WorldTransform* transform, const SceneObject& src);

	// コライダーを生成してオブジェクトに登録
	static void ApplyCollider(Object3d* object, const std::string& name, const ColliderInfo& col);

	// 通常オブジェクトを生成してマネージャーに登録
	static void BuildObject(const SceneObject& sceneObj, std::vector<SceneObject>& outPendingEvents);

	// イベントオブジェクトを生成してマネージャーに登録
	static void BuildEvent(const SceneObject& sceneObj);
};

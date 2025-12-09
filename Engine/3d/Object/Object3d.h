#pragma once
#include "Object3dManager.h"
#include "math/Vector3.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include "math/function.h"
#include <fstream>
#include "Model/Model.h"
#include "Model/BaseModel.h"
#include <3d/Camera/Camera.h>
#include <3d/Object/Renderer/BaseRenderer.h>
#include <3d/Collider/BaseCollider.h>
#include "ObjectData.h"
class Object3dManager;
class WorldTransform;

struct TimeData {
	float max;
	float current;
};

class Object3d
{
public: // メンバ関数
	Object3d(std::string objectName);
	virtual ~Object3d() = default;
	// 初期化処理
	virtual void Initialize();
	// 更新処理
	virtual void Update();

	virtual void Draw();

	void ResetObject();

#ifdef _DEBUG
	virtual void DebugGui();
#endif // _DEBUG

	// 衝突した
	virtual void OnCollisionEnter([[maybe_unused]] BaseCollider* other);
	// 衝突中
	virtual void OnCollisionStay([[maybe_unused]] BaseCollider* other);
	// 離れた
	virtual void OnCollisionExit([[maybe_unused]] BaseCollider* other);

private: // メンバ変数
	Object3dManager* objectManager_ = nullptr;
	Camera* camera_ = nullptr;
	std::unique_ptr<WorldTransform> transform_;

	std::vector<BaseRenderer*> renders_;
	std::vector<BaseCollider*> colliders_;

	// どうやって描画するかの設定
	DrawOption drawOption_;

public: // ゲッター // セッター // 
	// レンダー追加処理
	void AddRenderer(BaseRenderer* render);
	void AddCollider(BaseCollider* collider);

	BaseRenderer* GetRenderer(std::string name_);
	BaseCollider* GetCollider(std::string name_);
	// カメラ
	void SetCamera(Camera* camera) { camera_ = camera; }

	DrawOption& GetOption() { return drawOption_; }

	// ワールドトランスフォームの取得
	WorldTransform* GetWorldTransform() { return transform_.get(); }

	std::string name_;

	bool isAlive = true;
};
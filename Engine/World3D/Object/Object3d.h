#pragma once
#include "Object3dManager.h"
#include "Math/Vector3.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Math/MathUtils.h"
#include <fstream>
#include "Model/Model.h"
#include "Model/BaseModel.h"
#include "World3D/Camera/BaseCamera.h"
#include <World3D/Object/Renderer/BaseRenderer.h>
#include <World3D/Object/Renderer/IDeferredDrawable.h>
#include <World3D/Object/Renderer/IShadowCaster.h>
#include <World3D/Collider/BaseCollider.h>
#include "ObjectData.h"
class Object3dManager;
class WorldTransform;

struct TimeData {
	float max = 1.0f;
	float current = 0.0f;
};

class Object3d
{
public: // メンバ関数
	Object3d(std::string objectName);
	virtual ~Object3d() = default;
	// 初期化処理
	virtual void Initialize();
	// 更新処理
	virtual void Update(float deltaTime);
	virtual void Draw();

	void DrawShadow();

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
	BaseCamera* camera_ = nullptr;
	std::unique_ptr<WorldTransform> transform_;

	std::vector<BaseRenderer*> renders_;
	std::vector<IDeferredDrawable*> deferredDrawables_;
	std::vector<IShadowCaster*> shadowCasters_;
	std::vector<BaseCollider*> colliders_;

	// どうやって描画するかの設定
	struct DrawOption {
		BlendMode blendMode = BlendMode::kNormal;
		DrawPath drawPath = DrawPath::Deferred;
	}drawOption_;
	// 描画するかどうかの設定
	bool isDraw = true;

public: // ゲッター // セッター // 
	// レンダー追加処理
	void AddRenderer(BaseRenderer* render);
	void AddCollider(BaseCollider* collider);

	BaseRenderer* GetRenderer(std::string name_);
	BaseCollider* GetCollider(std::string name_);
	// カメラ
	void SetCamera(BaseCamera* camera) { camera_ = camera; }

	DrawOption& GetOption() { return drawOption_; }

	void SetIsDraw(bool flag) { isDraw = flag; }

	// ワールドトランスフォームの取得
	WorldTransform* GetWorldTransform() { return transform_.get(); }

	std::string name_;

	bool isAlive = true;
};
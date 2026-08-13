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

	// スキンモデルのCSスキニングを回す。
	// 影・GBuffer・Forwardのどれからも同じ結果を使うので、全描画パスより前に1回だけ呼ぶ
	void DispatchSkinning();

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

	// ステージデータに書き出す型名。Object3dFactory::Create が入れる
	std::string className_ = "Object3d";
	// ステージデータとして保存する対象か
	bool isStageObject_ = false;
	// 使用するモデル名（ステージデータの "model"）
	std::string modelName_;

public: // ゲッター // セッター // 
	// レンダー追加処理
	void AddRenderer(BaseRenderer* render);
	void AddCollider(BaseCollider* collider);
	// コライダーを外す。CollisionManager 側の実体も次のフレームで破棄される
	void RemoveCollider(BaseCollider* collider);

	BaseRenderer* GetRenderer(std::string name_);
	BaseCollider* GetCollider(std::string name_);

	// エディタ（Hierarchy / Inspector）が中身を一覧するために使う。読むだけ
	const std::vector<BaseRenderer*>& GetRenderers() const { return renders_; }
	const std::vector<BaseCollider*>& GetColliders() const { return colliders_; }
	// カメラ
	void SetCamera(BaseCamera* camera) { camera_ = camera; }

	DrawOption& GetOption() { return drawOption_; }

	void SetIsDraw(bool flag) { isDraw = flag; }
	bool GetIsDraw() const { return isDraw; }

	// ワールドトランスフォームの取得
	WorldTransform* GetWorldTransform() { return transform_.get(); }

	// --- ステージデータ（保存 / 読み込み）用 ---

	// "Ground" などの型名。Object3dFactory::Create が入れる
	const std::string& GetClassName() const { return className_; }
	void SetClassName(const std::string& className) { className_ = className; }

	// ステージデータとして保存する対象か。
	// 敵の武器のように実行中に生えるオブジェクトまで保存しないよう、
	// SceneBuilder とエディタの生成経路だけが true にする
	bool IsStageObject() const { return isStageObject_; }
	void SetStageObject(bool isStageObject) { isStageObject_ = isStageObject; }

	// 使用するモデル名（ステージデータの "model"）。
	// Ground / Prop は Initialize() でこれを読んでレンダラーを作る。
	// 素の Object3d でも、エディタでモデルを貼った場合はここに入る。
	// 生成時に指定する場合は Initialize() より前に呼ぶこと
	// （実行中の差し替えは Inspector が ModelRenderer::SetModel と併せて行う）
	const std::string& GetModelName() const { return modelName_; }
	void SetModelName(const std::string& modelName) { modelName_ = modelName; }

	std::string name_;

	bool isAlive = true;
};
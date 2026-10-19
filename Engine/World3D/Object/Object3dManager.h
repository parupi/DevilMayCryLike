#pragma once
#include "Graphics/Device/DirectXManager.h"
#include "World3D/Camera/BaseCamera.h"
#include <memory>
#include <mutex>
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "World3D/Object/Renderer/StaticInstanceBatcher.h"

class Object3d;
class Object3dManager
{
private:
	Object3dManager() = default;
	Object3dManager(const Object3dManager&) = delete;
	Object3dManager& operator=(const Object3dManager&) = delete;
public:
	// シングルトンインスタンスの取得
	static Object3dManager& GetInstance();
	// 初期化
	void Initialize(DirectXManager* directXManager, PSOManager* psoManager);
	// 終了
	void Finalize();
	// 更新
	void Update();
	// スキンモデルのCSスキニングを全オブジェクトぶん回す。
	// 影もGBufferも同じ出力頂点バッファを読むので、すべての描画パスより前に1回だけ実行する
	void DispatchSkinning();

	// 描画前処理
	void DrawForward();
	void DrawDeferred();
	// 影を描画。カスケードのライトVPを渡すと、範囲外のオブジェクトを省いて描く
	void DrawShadow(const Matrix4x4& lightViewProj);

	void AddObject(std::unique_ptr<Object3d> object);

	void DeleteObject(const std::string& name);

	void DeleteAllObject();
	// 生存フラグを切ったオブジェクトの削除
	void RemoveDeadObject();

	Object3d* FindObject(std::string objectName);

	std::vector<Object3d*> GetAllObject();
	// 時間を設定する
	void SetDeltaTime(float deltaTime) { deltaTime_ = deltaTime; }

	// 描画の最初（影のパスより前）に1回呼ぶ。まとめて描く用のバッファを先頭へ戻す
	void BeginRenderFrame();
	// 動かない同じモデルをまとめて描く仕組み（ModelRenderer::SetInstanced で対象になる）
	StaticInstanceBatcher& GetInstanceBatcher() { return *instanceBatcher_; }

	/// <summary>
	/// 境界球が今のカメラの視錐台に入っているか（GBuffer を描く間だけ有効。DrawDeferred の頭で平面を作る）。
	/// カリングを切っている・カメラが無いときは常に true
	/// </summary>
	bool IsInViewFrustum(const Vector3& center, float radius) const;
	// カメラの視錐台カリングの有効・無効（Render ウィンドウで切り替えて見比べる用）
	void SetFrustumCullingEnabled(bool enabled) { frustumCullingEnabled_ = enabled; }
	bool IsFrustumCullingEnabled() const { return frustumCullingEnabled_; }
	// 直近の GBuffer で、描いた数・省いた数（カリング対象の境界球を持つものだけ数える）
	uint32_t GetLastDrawnCount() const { return lastDrawnCount_; }
	uint32_t GetLastCulledCount() const { return lastCulledCount_; }
	// Object3d::Draw がカリングの結果を数えるのに使う
	void CountCulling(bool drawn) const { drawn ? ++drawnCount_ : ++culledCount_; }
private:
	// viewProj（行ベクトル規約）から視錐台の6平面を作る
	void BuildViewFrustum(const Matrix4x4& viewProj);

	std::unique_ptr<StaticInstanceBatcher> instanceBatcher_;

	bool frustumCullingEnabled_ = true;
	bool frustumValid_ = false;
	// 平面 (a, b, c, d)。内側で a*x + b*y + c*z + d >= 0、(a, b, c) は長さ1
	float frustumPlanes_[6][4]{};
	mutable uint32_t drawnCount_ = 0;
	mutable uint32_t culledCount_ = 0;
	uint32_t lastDrawnCount_ = 0;
	uint32_t lastCulledCount_ = 0;
private:
	// DirectXのポインタ
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	// カメラのポインタ
	BaseCamera* defaultCamera_ = nullptr;

	BlendMode blendMode_ = BlendMode::kNone;

	std::vector<std::unique_ptr<Object3d>> objects_;

	float deltaTime_ = 0.0f;

public: // ゲッター // セッター //
	DirectXManager* GetDxManager() const { return dxManager_; }
	PSOManager* GetPsoManager() const { return psoManager_; }
	// デフォルトカメラ
	void SetDefaultCamera(BaseCamera* camera) { defaultCamera_ = camera; }
	BaseCamera* GetDefaultCamera() const { return defaultCamera_; }
};


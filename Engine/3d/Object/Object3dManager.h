#pragma once
#include "Graphics/Device/DirectXManager.h"
#include "3d/Camera/BaseCamera.h"
#include <memory>
#include <mutex>
#include "Graphics/Rendering/PSO/PSOManager.h"

class Object3d;
class Object3dManager
{
private:
	static Object3dManager* instance;
	static std::once_flag initInstanceFlag;

	Object3dManager() = default;
	~Object3dManager() = default;
	Object3dManager(Object3dManager&) = default;
	Object3dManager& operator=(Object3dManager&) = default;
public:
	// シングルトンインスタンスの取得
	static Object3dManager* GetInstance();
	// 初期化
	void Initialize(DirectXManager* directXManager, PSOManager* psoManager);
	// 終了
	void Finalize();
	// 更新
	void Update();
	// 描画前処理
	void DrawForward();
	void DrawDeferred();
	// 影を描画
	void DrawShadow();

	// アニメーション用描画前処理
	void DrawSetForAnimation();

	void AddObject(std::unique_ptr<Object3d> object);

	void DeleteObject(const std::string& name);

	void DeleteAllObject();
	// 生存フラグを切ったオブジェクトの削除
	void RemoveDeadObject();

	Object3d* FindObject(std::string objectName);

	std::vector<Object3d*> GetAllObject();
	// 時間を設定する
	void SetDeltaTime(float deltaTime) { deltaTime_ = deltaTime; }
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


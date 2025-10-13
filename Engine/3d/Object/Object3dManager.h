#pragma once
#include "base/DirectXManager.h"
#include <3d/Camera/Camera.h>
#include <memory>
#include <mutex>
#include <base/PSOManager.h>

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
	void DrawSet();
	// アニメーション用描画前処理
	void DrawSetForAnimation();

	void AddObject(std::unique_ptr<Object3d> object);

	void DeleteObject(const std::string& name);

	void DeleteAllObject();
	// 生存フラグを切ったオブジェクトの削除
	void RemoveDeadObject();

	Object3d* FindObject(std::string objectName);

	std::vector<Object3d*> GetAllObject();
private:
	// DirectXのポインタ
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	// カメラのポインタ
	Camera* defaultCamera_ = nullptr;

	BlendMode blendMode_ = BlendMode::kNone;

	std::vector<std::unique_ptr<Object3d>> objects_;

public: // ゲッター // セッター //
	DirectXManager* GetDxManager() const { return dxManager_; }
	PSOManager* GetPsoManager() const { return psoManager_; }
	// デフォルトカメラ
	void SetDefaultCamera(Camera* camera) { defaultCamera_ = camera; }
	Camera* GetDefaultCamera() const { return defaultCamera_; }
};


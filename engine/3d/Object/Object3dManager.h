#pragma once
#include "DirectXManager.h"
#include <Camera.h>
#include <memory>
#include <mutex>
#include "PSOManager.h"

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
	// 描画前処理
	void DrawSet(BlendMode blendMode);
	// アニメーション用描画前処理
	void DrawSetForAnimation();

	void AddObject(std::unique_ptr<Object3d> object);

private:
	// DirectXのポインタ
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	// カメラのポインタ
	Camera* defaultCamera_ = nullptr;

	std::vector<std::unique_ptr<Object3d>> objects_;

public: // ゲッター // セッター //
	DirectXManager* GetDxManager() const { return dxManager_; }
	// デフォルトカメラ
	void SetDefaultCamera(Camera* camera) { defaultCamera_ = camera; }
	Camera* GetDefaultCamera() const { return defaultCamera_; }
};


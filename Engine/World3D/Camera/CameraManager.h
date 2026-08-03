#pragma once
#include <map>
#include <memory>
#include "World3D/Camera/BaseCamera.h"
#include <mutex>
#include "Graphics/Device/DirectXManager.h"

class CameraManager
{
private:
	CameraManager() = default;
	CameraManager(const CameraManager&) = delete;
	CameraManager& operator=(const CameraManager&) = delete;
public:
	// シングルトンインスタンスの取得
	static CameraManager& GetInstance();
	// 初期化
	void Initialize(DirectXManager* dxManager);
	// 終了
	void Finalize();
	// カメラを追加する
	void AddCamera(std::unique_ptr<BaseCamera> camera);

	// アクティブなカメラを更新する
	void Update();

	// アクティブなカメラを設定する
	void SetActiveCamera(const std::string& cameraName, float transitionTime = 0.0f);

	// アクティブなカメラを取得する
	BaseCamera* GetActiveCamera() const;

	// 現在のカメラを取得する
	BaseCamera* GetCurrentCamera() const;

	// アクティブなカメラをシェーダーに送る
	void BindCameraToShader();
	// 今ある全てのカメラを消す
	void DeleteAllCamera();
	// 指定した名前のカメラを消す
	void RemoveCamera(const std::string& name);

	// 今カメラ切り替えをしているかどうかの判定
	bool IsTransition() const { return isTransitioning_; }

	// 名前からカメラを探す
	BaseCamera* FindCamera(const std::string& name) { return cameras_[name].get(); }

	// ── エディタ用 ──
	// 登録済みカメラ名の一覧（昇順）
	std::vector<std::string> GetCameraNames() const;
	const std::string& GetActiveCameraName() const { return activeCameraName_; }

#ifdef _DEBUG
	/// <summary>
	/// エディタのデバッグカメラを割り込ませる（nullptr で解除）。
	///
	/// 立っている間、GetActiveCamera() / GetCurrentCamera() は無条件にこれを返すので、
	/// 描画・パーティクル・当たり判定まで下流すべてがデバッグカメラを見る。
	/// ゲーム側のカメラも裏で更新され続けるため、解除したときに絵が飛ばない。
	/// カメラの実体はエディタ（EditorCamera）が持つ。cameras_ には入れない
	/// （DeleteAllCamera() でシーンごと消えてしまうため）。
	/// </summary>
	void SetDebugCamera(BaseCamera* camera);
	BaseCamera* GetDebugCamera() const { return debugCamera_; }
	bool IsDebugCameraActive() const { return debugCamera_ != nullptr; }
#endif // _DEBUG

private:
	// 補間の更新
	void TransitionUpdate();
	void CreateCameraResource();
	// activeCameraName_ が指すカメラ。デバッグカメラの割り込みは見ない
	BaseCamera* FindActiveCameraEntry() const;

	// カメラ座標
	struct CameraForGPU {
		Vector3 worldPosition;
	};
private:
	// カメラを名前で管理
	std::unordered_map<std::string, std::unique_ptr<BaseCamera>> cameras_;
	// 現在のアクティブカメラ名
	std::string activeCameraName_;
	// 切り替わる先のカメラ名
	std::string nextCameraName_;

	DirectXManager* dxManager_ = nullptr;

	uint32_t cameraHandle_ = 0;

	CameraForGPU* cameraData_ = nullptr;

	// 切り替え保管用のカメラ
	std::unique_ptr<BaseCamera> transitionCamera_ = nullptr;

#ifdef _DEBUG
	// エディタが持つデバッグカメラ。所有はしない
	BaseCamera* debugCamera_ = nullptr;
#endif

	// 補間関連
	bool isTransitioning_ = false;
	float transitionTime_ = 0.0f;
	float transitionTimer_ = 0.0f;
	Vector3 startPos_;
	Vector3 endPos_;
	Vector3 startRot_;
	Vector3 endRot_;
};
#pragma once
#include "3d/Camera/BaseCamera.h"

/// <summary>
/// タイトル画面で使用されるカメラを管理するクラス
/// カメラの移動・回転を状態に応じて制御する
/// </summary>
class TitleCamera : public BaseCamera
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="objectName">カメラオブジェクト名</param>
	TitleCamera(std::string objectName);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~TitleCamera() override = default;

	/// <summary>
	/// 初期化処理
	/// カメラの初期位置や状態を設定する
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新処理
	/// 現在の状態に応じてカメラを制御する
	/// </summary>
	void Update() override;

	/// <summary>
	/// タイトル演出の開始処理（カメラ移動の開始）
	/// </summary>
	void Enter();

	/// <summary>
	/// タイトル演出の終了処理（次のシーンへの遷移準備）
	/// </summary>
	void Exit();

	/// <summary>
	/// カメラ演出が終了しているかどうかを取得
	/// </summary>
	/// <returns>終了している場合はtrue</returns>
	bool IsExit() const { return isExit_; }

private:
	/// <summary>
	/// タイトルカメラの状態を示す列挙型
	/// </summary>
	enum class TitleState {
		Enter,	// 演出開始
		Idle,	// 停止中
		Exit,	// 終了処理中
	} titleState_ = TitleState::Idle;

	/// <summary>
	/// 状態ごとの目標位置
	/// </summary>
	Vector3 targetTranslate_;
	/// <summary>
	/// 状態ごとの目標回転
	/// </summary>
	Vector3 targetRotate_;
	/// <summary>
	/// 状態開始時の位置
	/// </summary>
	Vector3 startTranslate_;
	/// <summary>
	/// 状態開始時の回転
	/// </summary>
	Vector3 startRotate_;

	/// <summary>
	/// 状態経過時間
	/// </summary>
	float stateTimer_ = 0.0f;
	/// <summary>
	/// 状態にかける時間
	/// </summary>
	float stateTime_ = 5.0f;

	/// <summary>
	/// 演出が終了しているかどうか
	/// </summary>
	bool isExit_ = false;
};

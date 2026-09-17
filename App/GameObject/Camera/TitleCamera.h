#pragma once
#include "World3D/Camera/BaseCamera.h"

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
	/// 導入のカメラ移動を打ち切って、その場で待機状態へ移す。
	/// 導入中に入力されたときに呼ぶ（1回目の入力でスキップ、2回目でゲーム開始）
	/// </summary>
	void SkipEnter();

	/// <summary>
	/// タイトル演出の終了処理（次のシーンへの遷移準備）
	/// </summary>
	void Exit();

	/// <summary>
	/// カメラ演出が終了しているかどうかを取得
	/// </summary>
	/// <returns>終了している場合はtrue</returns>
	bool IsExit() const { return isExit_; }

	/// <summary>
	/// 導入のカメラ移動中かどうか
	/// </summary>
	bool IsEntering() const { return titleState_ == TitleState::Enter; }

	/// <summary>
	/// 入力待ちの待機状態かどうか
	/// </summary>
	bool IsIdle() const { return titleState_ == TitleState::Idle; }

private:
	/// <summary>
	/// 待機状態へ入る。今の位置を漂いの中心として覚える
	/// </summary>
	void BeginIdle();

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

	// ==========================
	// 待機中の漂い
	// ==========================
	/// <summary>
	/// 待機に入ってからの経過時間（漂いの位相に使う）
	/// </summary>
	float idleTimer_ = 0.0f;
	/// <summary>
	/// 漂いの中心になる位置（Enterの到達点）
	/// </summary>
	Vector3 idleBaseTranslate_{};
	/// <summary>
	/// 漂いの中心になる回転
	/// </summary>
	Vector3 idleBaseRotate_{};

	/// <summary>
	/// シーン切り替えを要求済みかどうか。
	/// Exitの判定は毎フレーム成立するので、要求が何度も飛ばないよう見張る
	/// </summary>
	bool sceneChangeRequested_ = false;

	// ==========================
	// 演出パラメータ
	// ==========================
	/// <summary>導入のカメラ移動にかける秒数</summary>
	static constexpr float kEnterTime = 4.0f;
	/// <summary>決定してから飛び込みきるまでの秒数</summary>
	static constexpr float kExitTime = 1.5f;
	/// <summary>Exitのうち、この割合を過ぎたらシーン切り替えを要求する</summary>
	static constexpr float kExitSceneChangeRate = 0.6f;
};

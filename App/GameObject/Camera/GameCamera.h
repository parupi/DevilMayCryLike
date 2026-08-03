#pragma once
#include "World3D/Camera/BaseCamera.h"

class Player;
class LockOnSystem;
class CameraInput;
class GlobalVariables;

class GameCamera : public BaseCamera {
public:
	enum class Mode {
		Free,
		LockOn
	};

	// ⑫ 状態管理：戦闘の有無で距離・FOVなどを切り替える
	enum class State {
		Normal, // 探索中
		Battle  // ロックオン中、または敵が近くに見えている
	};

	GameCamera(std::string cameraName);
	~GameCamera() override = default;

	void Initialize(Player* player, LockOnSystem* lockOn, CameraInput* cameraInput);
	void Update() override;

	// モードの切り替え
	void SetMode(Mode mode);

	/// <summary>
	/// カメラシェイクを加える（トラウマ加算方式）。被弾・着地・爆発などから呼ぶ。
	/// </summary>
	/// <param name="trauma">加えるトラウマ量（0〜1目安）。実際の揺れはtrauma^2に比例する</param>
	void AddShake(float trauma);

#ifdef _DEBUG
	/// <summary>
	/// エディタ表示用の状態スナップショット。
	/// パラメータ自体は GlobalVariables 側にあるので App/Editor/Windows/CameraWorkWindow.cpp が
	/// 直接いじる。ここで渡すのは内部の実行時状態だけ。
	/// </summary>
	struct EditorStatus {
		Mode mode;
		State state;
		float battleBlend;
		float actionZoomScale;
		float shakeTrauma;
	};
	EditorStatus MakeEditorStatus() const {
		return { mode_, state_, battleBlend_, actionZoomScale_, shakeTrauma_ };
	}
#endif

	/// <summary>
	/// 追従位置（プレイヤーの背後）へ補間なしで即座に移動する。
	/// CameraManager はカメラ切り替えの補間先を SetActiveCamera を呼んだ時点の位置で記録するため、
	/// スタート演出の「引き」の到達点を正しくするには、切り替え前にこれを呼んでおく必要がある。
	/// </summary>
	/// <param name="playerForward">プレイヤーの向き（水平・単位ベクトル）</param>
	void SnapToFollow(const Vector3& playerForward);

private:
	void UpdateFree();
	void UpdateLockOn();
	// GlobalVariablesエディタに調整項目を登録し、初期値を反映する（Initializeから呼ぶ）
	void RegisterParams();
	// GlobalVariablesの現在値を各メンバへ反映する（毎フレーム先頭で呼ぶ）
	void ApplyParams();
	// トラウマの減衰・シェイクオフセットの計算と、被弾/着地トリガーの検出
	void UpdateShake(float dt);
	// ⑧FOV変化・⑨Action Camera（攻撃時ズーム）を速度/攻撃状態から更新する
	void UpdateFovAndZoom(float dt);
	// ⑫ Normal/Battle状態を判定し、状態ブレンド値を滑らかに更新する
	void UpdateCameraState(float dt);
	// yaw/pitch/distance から、プレイヤーに対する追従位置を求める
	Vector3 CalcDesiredPosition(const Vector3& playerPos) const;
	// 注視点を滑らかに追従させてからLookAtする（モード切替時の視点飛びを防ぐ）
	void ApplySmoothLookAt(const Vector3& lookTarget);
	// ピボット→希望位置の間にGroundコライダー（壁・床）があれば、カメラを遮蔽物の手前へ引き寄せる
	Vector3 ResolveCameraCollision(const Vector3& pivot, const Vector3& desiredPos) const;

private:
	Player* player_ = nullptr;
	LockOnSystem* lockOn_ = nullptr;
	CameraInput* cameraInput_ = nullptr;
	GlobalVariables* global_ = nullptr;

	Mode mode_ = Mode::Free;
	State state_ = State::Normal;
	// Battleへの寄り具合[0,1]。状態切替を滑らかにするためのブレンド値
	float battleBlend_ = 0.0f;

	// ===== 実行時の状態（GlobalVariablesでは管理しない） =====
	float yaw_ = 3.14f;
	float pitch_ = 0.0f;
	float distance_ = 18.0f;

	Vector3 smoothedLookOffset_ = Vector3(0.0f, 0.0f, 0.0f);

	Vector3 smoothedLookTarget_ = Vector3(0.0f, 0.0f, 0.0f);
	bool lookTargetInitialized_ = false;

	Vector3 velocity_ = Vector3(0.0f, 0.0f, 0.0f);

	// 右スティック無入力が続いた時間（自動補正の発動判定用）
	float autoRotateTimer_ = 0.0f;

	// ⑩ Camera Shake の実行時状態
	float shakeTrauma_ = 0.0f;                          // 現在のトラウマ量[0,1]
	Vector3 shakeOffset_ = Vector3(0.0f, 0.0f, 0.0f);   // 今フレームのシェイクオフセット
	int32_t prevHp_ = 0;                                // 被弾検出用の前フレームHP
	bool prevOnGround_ = true;                          // 着地検出用の前フレーム接地状態
	float prevFallSpeed_ = 0.0f;                        // 着地強度判定用の前フレーム落下速度

	// ⑨ Action Camera：攻撃状態から滑らかに寄る/戻るためのズーム倍率（実行時）
	float actionZoomScale_ = 1.0f;

	// ===== 調整パラメータ（GlobalVariablesエディタから編集） =====
	// 既定値は RegisterParams() の AddItem と一致させること
	float baseFollowDistance_ = 18.0f;// Free時の基準追従距離（distance_の目標値）
	float distanceLerpSpeed_ = 4.0f;  // 実行時距離が基準へ寄る速度
	float baseHeight_ = 3.0f;         // 追従位置の基準高さ
	float minDistance_ = 5.0f;        // プレイヤーに寄りすぎない最小距離
	float sensitivityX_ = 0.03f;      // 右スティック水平感度
	float sensitivityY_ = 0.025f;     // 右スティック垂直感度
	float pitchLimit_ = 1.2f;         // pitchの上下限（ラジアン）

	// ⑤ Auto Rotation（スティック放置＋移動中に背後へ戻る）
	bool autoRotateEnabled_ = true;
	float autoRotateDelay_ = 0.7f;    // 無入力からこの秒数経過で発動
	float autoRotateSpeed_ = 3.0f;    // 背後へ戻る補間速度
	float autoRotateMoveSpeed_ = 0.5f;// この水平速度以上で移動中とみなす

	float positionLagSpeed_ = 5.0f;   // カメラ位置の追従速度
	float lookLagSpeed_ = 2.5f;       // 前方注視オフセットの追従速度
	float lookForwardOffset_ = 3.0f;  // プレイヤー前方への注視オフセット量
	float lookHeight_ = 2.0f;         // 注視点・衝突ピボットの高さ
	float lookPitchDip_ = 4.0f;       // pitchに応じた注視点の下げ量
	float lookTargetLagSpeed_ = 8.0f; // 注視点そのものの追従速度

	float collisionMargin_ = 0.4f;    // 遮蔽物の手前に確保する余白
	float collisionMinDist_ = 1.0f;   // 衝突補正時にピボットから離す最小距離

	float lockOnDistanceMul_ = 1.2f;  // 敵との距離に対するカメラ距離倍率
	float lockOnDistanceMin_ = 12.0f; // ロックオン時の最小距離
	float lockOnDistanceMax_ = 25.0f; // ロックオン時の最大距離
	float lockOnHeight_ = 8.0f;       // ロックオン時のカメラ高さ
	float lockOnRightOffset_ = 3.0f;  // ロックオン時の横方向オフセット
	float lockOnLagSpeed_ = 5.0f;     // ロックオン時のカメラ追従速度

	// ⑧ FOV変化（速度で画角を広げる）
	float fovNormal_ = 0.45f;         // 通常時の水平FOV
	float fovDash_ = 0.6f;            // 高速移動時の水平FOV
	float fovSpeedMin_ = 6.0f;        // FOVが広がり始める水平速度
	float fovSpeedMax_ = 14.0f;       // FOVが最大まで広がる水平速度
	float fovLerpSpeed_ = 4.0f;       // FOVの補間速度

	// ⑨ Action Camera（攻撃時ズーム）
	float attackDistanceScale_ = 0.85f;// 攻撃中の距離倍率（<1で寄る）
	float actionZoomSpeed_ = 6.0f;    // ズーム倍率の補間速度

	// ⑪ Enemy Framing（Free時、敵が画面端に寄ったら軽くYaw補正）
	bool enemyFramingEnabled_ = true;
	float enemyFramingEdge_ = 0.55f;  // |ndc.x|がこの値を超えたら補正を始める(0〜1)
	float enemyFramingYawSpeed_ = 0.6f;// 画面端での最大Yaw補正速度(rad/秒)

	// ⑫ CameraState（Normal/Battle）
	bool battleStateEnabled_ = true;
	float battleDistanceScale_ = 0.92f;// Battle時の距離倍率（<1で寄る）
	float battleFovAdd_ = 0.05f;      // Battle時に加える水平FOV
	float battleBlendSpeed_ = 3.0f;   // 状態ブレンドの補間速度

	// ⑩ Camera Shake
	float shakeMaxOffset_ = 1.2f;         // トラウマ1.0時の最大オフセット量
	float shakeDecayRate_ = 1.5f;         // トラウマの減衰速度（/秒）
	float shakeHitTrauma_ = 0.6f;         // 被弾時に加えるトラウマ
	float shakeLandTrauma_ = 0.35f;       // 着地時に加えるトラウマ
	float shakeLandSpeedThreshold_ = 8.0f;// この落下速度以上の着地でシェイク
};
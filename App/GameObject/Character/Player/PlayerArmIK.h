#pragma once
#include <string>
#include <vector>
#include <Math/Vector3.h>
#include <Math/Quaternion.h>
#include "World3D/Object/Model/Animation/BoneAttachment.h"
#include "World3D/Object/Model/Animation/IKSolver.h"

class BaseRenderer;

/// <summary>
/// 攻撃を振っている間だけ、剣を握る手へ腕を向ける CCD IK。
///
/// **剣は一切動かさない。** 剣の姿勢は今まで通り攻撃の制御点（CatmullRom）が決めていて、
/// こちらはその剣を「握るとしたら手はここ」という点を目標に腕を解く。
/// 剣は腕よりずっと遠くまで振られるので、たいていの場合 IK は届かない。
/// CCD は届かない目標に対してチェーンが目標の方向へ伸びきった姿勢に収束するので、
/// 結果として **腕が剣の方を向く**。この用途ではそれが狙いどおりの挙動になる。
///
/// 目標はプレイヤーのローカル空間で受け取り、内部でスケルトン（モデル）空間へ変換する。
/// モデルはレンダラー側で 0.38 倍・足元オフセット付きで置かれているので、
/// 変換を挟まずにボーンへ渡すと、まったく違う場所を指す。
/// </summary>
class PlayerArmIK
{
public:
	/// <summary>プレイヤーのモデルレンダラーを渡す。スキンモデルでなければ何もしない状態になる</summary>
	void Initialize(BaseRenderer* modelRenderer);

	/// <summary>
	/// 毎フレーム。**スキンのポーズ更新より前**（Player::Update なら Object3d::Update の手前）に呼ぶ。
	/// holdPosition / holdRotation は PlayerWeapon::GetHoldPoseLocal の値。
	/// wantIK が false の間はウェイトが 0 へ落ちていき、0 になったら IK を外す
	/// </summary>
	void Update(bool wantIK, const Vector3& holdPositionLocal, const Quaternion& holdRotationLocal, float deltaTime);

	float GetWeight() const { return weight_; }
	bool IsReady() const { return ready_; }

#ifdef _DEBUG
	/// <summary>調整UI（Player ウィンドウから呼ばれる）</summary>
	void DrawEditor();
	/// <summary>目標と腕のチェーンをワールドに線で描く。デバッグ描画のON/OFFは中の設定で持つ</summary>
	void DrawDebug(const class WorldTransform* playerTransform) const;
#endif // _DEBUG

private:
	void LoadParams();

	BaseRenderer* renderer_ = nullptr;
	// モデル空間への変換を引くためだけに持つ（追従はさせない）
	BoneAttachment space_;
	bool ready_ = false;

	// 0=アニメーションのまま、1=IK の結果そのまま。振りの出入りで滑らかに動かす
	float weight_ = 0.0f;
	// 直近の到達誤差[モデル空間]。エディタ表示用
	float lastError_ = 0.0f;
	// 直近の目標（プレイヤーのローカル空間）。デバッグ描画用
	Vector3 lastTargetLocal_{};

	// ── GlobalVariables "PlayerArmIK"（Resource/GlobalVariables/Player/PlayerArmIK.json）──
	bool enabled_ = true;
	float strength_ = 1.0f;       // 効き具合の上限
	float blendSpeed_ = 20.0f;    // 振りの出入りでウェイトが動く速さ
	float wristWeight_ = 1.0f;    // 手首を剣の向きへ合わせる強さ
	int32_t iterations_ = 6;
	float maxAngleDegrees_ = 90.0f;
	bool drawDebug_ = false;

	// 根→末端の順。effector は目標へ寄せたい手
	std::vector<std::string> chainJoints_{ "Shoulder.R", "UpperArm.R", "LowerArm.R" };
	std::string effectorJoint_ = "Palm.R";

	static constexpr const char* kGroupName = "PlayerArmIK";
	static constexpr const char* kDirectoryName = "Player";
	static constexpr int32_t kMaxChainJoints = 4;
};

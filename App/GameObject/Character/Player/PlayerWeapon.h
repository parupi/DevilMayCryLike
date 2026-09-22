#pragma once
#include "World3D/Object/Object3d.h"
#include "World3D/Object/Model/Animation/BoneAttachment.h"
#include <GameData/Score/StylishScoreManager.h>
#include <string>

class Player;
class BaseRenderer;
class PlayerWeapon : public Object3d
{
public:
	/// <summary>剣を今どこが動かしているか。Player が毎フレーム決めて SetHoldMode で渡す</summary>
	enum class HoldMode {
		Swing, ///< 攻撃の制御点が動かす（構え〜振り抜き）。こちらは触らない
		Hand,  ///< 手のボーンに持たせる（コンボの合間）
		Back,  ///< 背中に背負う（攻撃していない間）。位置は納刀モーション(Sheathe)の最後の制御点
	};

	PlayerWeapon(std::string objectName);
	~PlayerWeapon() override = default;
	// 初期
	void Initialize() override;
	// 更新
	void Update(float deltaTime) override;
	// 描画
	void Draw() override;

#ifdef _DEBUG
	void DebugGui() override;
#endif // _DEBUG
	// 衝突した
	void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;
	// 衝突中
	void OnCollisionStay([[maybe_unused]] BaseCollider* other) override;
	// 離れた
	void OnCollisionExit([[maybe_unused]] BaseCollider* other) override;

	// ======================
	// アクセッサ
	// ======================
	void SetIsAttack(bool flag) { isAttack_ = flag; }
	void SetPlayer(Player* player) { player_ = player; }
	void SetScoreManager(StylishScoreManager* scoreManager) { scoreManager_ = scoreManager; }
	/// <summary>
	/// 次の当たり判定を1回だけ切る（多段ヒット用）。
	/// 触れたままの敵に、もう一度 OnCollisionEnter を起こすために使う
	/// </summary>
	void RequestRehit() { rehitRequested_ = true; }

	/// <summary>刃先のワールド座標（今のワールド行列から計算する）。剣の軌跡とヒット位置に使う</summary>
	Vector3 GetBladeTipWorld();
	/// <summary>刃の根本（鍔のすぐ上）のワールド座標</summary>
	Vector3 GetBladeBaseWorld();

	// ======================
	// 手のボーンへ持たせる（Step0: 剣のボーン追従）
	// ======================
	/// <summary>
	/// 剣を手のボーン（既定は Palm.R）へ持たせる準備。プレイヤーのモデルレンダラーを渡す。
	/// 握りのオフセットは GlobalVariables の "PlayerWeapon" グループで調整する
	/// </summary>
	void InitializeGrip(BaseRenderer* modelRenderer);

	/// <summary>
	/// 持ち方（SetHoldMode）に合わせて、手または背中の姿勢へ寄せる。
	/// **スキンのポーズ更新より後**に呼ぶこと（Player::Update では Object3d::Update のすぐ後）。
	/// 振っている間（Swing）は攻撃の制御点が剣を動かすので何もしない
	/// </summary>
	void UpdateGrip(float deltaTime);

	/// <summary>
	/// 持ち方を切り替える。切り替えた瞬間に剣が飛ばないよう、今の姿勢から時間をかけて寄せ直す
	/// </summary>
	void SetHoldMode(HoldMode mode);
	HoldMode GetHoldMode() const { return holdMode_; }
	/// <summary>
	/// 背負ったときの姿勢（プレイヤーのローカル空間。回転は度）。
	/// 納刀モーション(Sheathe)の最後の制御点を渡す。制御点と同じ空間・同じ単位なので、
	/// 納刀モーションを振り終えた姿勢からそのまま背負った状態へ繋がる
	/// </summary>
	void SetBackPose(const Vector3& position, const Vector3& rotationDegree) {
		backPosition_ = position;
		backRotation_ = rotationDegree;
	}
	/// <summary>今の持ち方の姿勢へ寄っている割合（0=寄せ始め / 1=寄せ終わり）</summary>
	float GetGripWeight() const { return gripWeight_; }
	/// <summary>手のボーンに握りのオフセットを載せた姿勢。取れなければ false</summary>
	bool GetGripPose(Vector3& outPosition, Quaternion& outRotation) const;

	/// <summary>
	/// 「今の剣を握るなら手はここにある」という姿勢（プレイヤーのローカル空間）。
	/// GetGripPose のちょうど逆算で、腕IKの目標に使う。
	///
	/// 剣の姿勢は攻撃の制御点が決めているので、こちらは**剣を一切動かさない**。
	/// 行列ではなく今フレームの translation / rotation から組むこと
	/// （matWorld は UpdateTransformOnly を通るまで1フレーム古い）
	/// </summary>
	void GetHoldPoseLocal(Vector3& outPosition, Quaternion& outRotation) const;

#ifdef _DEBUG
	/// <summary>握りの調整UI（Player ウィンドウから呼ばれる）</summary>
	void DrawGripEditor();
#endif // _DEBUG

private:
	// GlobalVariables から握りの調整値を読む（エディタで動かしながら合わせられるよう毎フレーム）
	void LoadGripParams();
	bool isAttack_ = false;
	// RequestRehit で立てる。次の Update で判定を1回切ったら下ろす
	bool rehitRequested_ = false;
	// 当たり判定（OBB）の基準の大きさ。攻撃ごとの倍率（Hitbox Scale）はこれに掛ける
	Vector3 baseHalfExtents_ = { 0.5f, 1.0f, 0.5f };

	StylishScoreManager* scoreManager_;
	// プレイヤーの生ポインタ（武器からプレイヤーの状態を参照するために必要）
	Player* player_ = nullptr;
	// 移動と回転の初期位置
	Vector3 defaultPosition_ = { 0.0f, 0.6f, -0.5f };
	Vector3 defaultRotation_ = { 0.0f, 90.0f, 150.0f };

	// 刃先と刃の根本のローカル位置。Sword.obj は Y 方向に -1.31〜1.43 で、
	// -1.4〜-0.8 が柄、-0.8〜-0.7 が鍔、その上が刃（先端 1.43）
	Vector3 bladeTipOffset_ = { 0.0f, 1.35f, 0.0f };
	Vector3 bladeBaseOffset_ = { 0.0f, -0.65f, 0.0f };

	// ── 手のボーンへの追従 ──
	BoneAttachment grip_;
	BaseRenderer* gripRenderer_ = nullptr;
	// レンダラーが先にできていないことがあるので、スキンインスタンスが取れるまで初期化を待つ
	bool gripReady_ = false;
	// 最初は背負った状態（Initialize で置く既定の姿勢が納刀モーションの最後と同じ）
	HoldMode holdMode_ = HoldMode::Back;
	// 今の持ち方の姿勢へ寄せる割合。持ち方を変えたら 0 から上げ直して、剣が飛ばないようにする
	float gripWeight_ = 0.0f;

	// 背負ったときの姿勢。Player が納刀モーションの最後の制御点を毎フレーム渡してくる。
	// 渡されるまでは Initialize の置き場所（= 納刀モーションの最後と同じ値）を使う
	Vector3 backPosition_{};
	Vector3 backRotation_{};

	// GlobalVariables "PlayerWeapon"（Resource/GlobalVariables/Player/PlayerWeapon.json）の調整値。
	// 既定値は Alien.gltf のバインドポーズから出したもの。
	//   - Palm.R のローカル +Y は「腕が伸びる向き」。待機姿勢では腕が下がるので +Y は下を向く
	//   - 剣は Y 方向に約2.7と長いので、刃をそのまま +Y（下）へ向けると地面を突き抜ける。
	//     そこで X まわりに180度回して刃を腕と逆（＝上）へ向け、原点は手から -Y 側へ 1.1 ずらす。
	//     これで手が柄の中ほど（剣ローカルの y=-1.1／柄は -1.4〜-0.8）に来る
	std::string gripBoneName_ = "Palm.R";
	Vector3 gripOffsetPosition_ = { 0.0f, -1.1f, 0.0f };
	Vector3 gripOffsetRotation_ = { 180.0f, 0.0f, 0.0f };
	float gripBlendSpeed_ = 14.0f;
	bool gripEnabled_ = true;

	static constexpr const char* kGripGroupName = "PlayerWeapon";
	static constexpr const char* kGripDirectoryName = "Player";
};


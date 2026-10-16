#pragma once
#include <unordered_map>
#include <GameObject/Character/Player/State/Attack/PlayerStateAttack.h>
#include "GameObject/Character/Player/Controller/PlayerInput.h"
#include "AttackPlayer.h"
#include <memory>
#include <string>
#include <imgui.h>

enum class AttackType {
	Normal,
	RoundUp,
	LungeThrust,
	Air,
};

enum class StickDirection {
	None,
	ToEnemy,
	AwayFromEnemy,
	Any
};

struct AttackInputCondition
{
	InputButton button = InputButton::None;
	bool requireLockOn = false;
	StickDirection stick = StickDirection::None;
	// ジャスト回避の直後（カウンター受付中）だけ出せる攻撃か
	bool requireCounter = false;
};

struct AttackNode
{
	std::string name;
	bool isRootAttack = false;
	bool isAir = false;
	AttackInputCondition condition;
	// 派生先（攻撃名で管理）
	std::vector<std::string> nextAttacks;
};

class PlayerCombat
{
public:
	PlayerCombat() = default;
	~PlayerCombat() = default;
	// 初期化
	void Initialize(Player* player);
	// 更新
	void Update(float deltaTime);
	// 描画
	void Draw();
	// 攻撃を追加する
	void AddState(const std::string& stateName);
	// 現在攻撃中かどうか
	bool IsAttacking() const { return !currentState_.empty(); }
	// 溜め攻撃の溜め中か（体のモーションを構えで止めるのに使う）
	bool IsCharging() const { return !currentState_.empty() && currentState_.back()->IsCharging(); }
	// 今の攻撃（攻撃中でなければ nullptr）。演出が振りの段階を読むのに使う
	const PlayerStateAttack* GetCurrentAttack() const { return currentState_.empty() ? nullptr : currentState_.back(); }
	// 攻撃を強制中断する（被弾時など）。コンボも終了扱いになる
	void InterruptCombat();

	// ======================
	// コンボの区切り（敵の死亡待ちが見る）
	// ======================
	// コンボの途中か。最初の攻撃を出してから、次段へつながらずに終わる・中断されるまで true。
	// 納刀モーション(Sheathe)はコンボに含めない
	bool IsComboActive() const { return comboActive_; }
	// 今（または直前）のコンボの通し番号。新しいコンボを始めるたびに増える。
	// 「死亡待ちに入ったときのコンボが終わったか」を、知らせを受け取らずに番号の比較で判定できる
	uint32_t GetComboId() const { return comboId_; }
	// 攻撃を振り終えて、次の技（派生）か納刀モーションを待っている間か
	bool IsWaitingForNextCombo() const { return waitingForNextCombo_; }
	// 納刀モーション(Sheathe)か。振り終えた時点で剣は背中に収まっている
	bool IsSheatheAttack(const PlayerStateAttack& attack) const;
	/// <summary>
	/// 納刀モーションの最後の制御点（プレイヤーのローカル空間。回転は度）＝剣を背負ったときの姿勢。
	/// 攻撃エディタで直せば毎フレーム反映される。納刀モーションが無ければ false で、out は触らない
	/// </summary>
	bool GetSheathePose(Vector3& outPosition, Vector3& outRotation) const;
	// 攻撃ノードを取得
	const AttackNode& GetAttackNode(const std::string& name) const{ return attackGraph_.at(name); }
	// プレイヤーからのコマンドを受け取って処理する
	void ExecuteCommand(const PlayerCommand& command);
	// 現在アクティブな攻撃の名前を取得（攻撃中でなければ空文字）
	const std::string& GetCurrentAttackName() const {
		static const std::string kEmpty;
		return currentState_.empty() ? kEmpty : currentState_.back()->GetAttackName();
	}

	// ======================
	// カウンター（ジャスト回避の直後だけ出せる攻撃）
	// ======================
	// 受付を開く。ジャスト回避の成立時に呼ばれる。クールタイム中は開かない
	void OpenCounterWindow(float seconds);
	// 受付中か
	bool IsCounterWindowOpen() const { return counterWindowTimer_ > 0.0f; }

	// ======================
	// アクションガイド（ActionGuide）が見る読み取り専用の問い合わせ。
	// どれも状態を変えない。実際に出すかどうかは従来どおり ExecuteCommand / Update が決める
	// ======================
	/// <summary>
	/// このボタンとスティックの向きで、今ルート攻撃を探したら選ばれるもの（ExecuteCommand と同じ選び方）。無ければ nullptr
	/// </summary>
	const AttackNode* FindRootAttack(InputButton button, const Vector2& stickDir, bool onGround) const;
	/// <summary>攻撃中にこのボタンを押したときの、別のボタンのルート攻撃による割り込みの扱い</summary>
	enum class RootOverTiming {
		NotAllowed, ///< 割り込まない（今の攻撃の派生になる・同じボタンの連打・攻撃中でない）
		Now,        ///< すぐ割り込む
		Buffer,     ///< 硬直中なので覚えておき、割り込めるようになったら出す
	};
	RootOverTiming PeekRootAttackOver(InputButton button) const;
	// 別のボタンのルート攻撃の先行入力を覚えているか
	bool HasPendingRootAttack() const { return pendingRootTimer_ > 0.0f; }
	InputButton GetPendingRootButton() const { return pendingRootCommand_.button; }
	// 攻撃のつながり全体（ガイドが派生やルート攻撃を並べるのに使う）
	const std::unordered_map<std::string, AttackNode>& GetAttackGraph() const { return attackGraph_; }
	// 今の攻撃（ガイド用。const 版）
	const PlayerStateAttack* GetCurrentAttackState() const { return currentState_.empty() ? nullptr : currentState_.back(); }

#ifdef _DEBUG
	// 攻撃グラフと攻撃データはこのクラスの内部表現そのものなので、UIの中身はここに置いたまま。
	// ただしウィンドウの開閉と描画タイミングは App/Editor/Windows/AttackEditorWindow.cpp が握る
	void DrawAttackDataEditorUI();
	void DrawAttackDerivativeEditorUI();
	AttackPlayer* GetAttackPlayer() { return attackPlayer_.get(); }
#endif // _DEBUG

private:
	// Jsonの名前からステートを生成
	void CreateState();
	// 攻撃を追加
	void AddAttackState(const std::string& attackName);
	// 攻撃データエディタのUIを描画
	void DrawAttackDataEditor(PlayerStateAttack* attack);

	AttackNode LoadAttackNode(const std::string& attackName);

	/// <summary>
	/// 1つの攻撃のデータと、攻撃グラフの出す条件（派生先以外）を GlobalVariables から読み直す。
	/// 以前は毎フレーム全攻撃を読み直しており、それだけで Debug の1フレームの 5.7ms を使っていた。
	/// いまは起動時に1回読み、Debug ではエディタで開いている攻撃と振っている攻撃だけをこれで読み直す
	/// </summary>
	void RefreshAttack(const std::string& attackName);

	void DrawAttackNodeEditor(const std::string& attackName, AttackNode& node);

	// 入力に合うルート攻撃を探す（条件が多いほど優先）。無ければ nullptr
	const AttackNode* FindRootAttack(const PlayerCommand& command) const;
	// ルート攻撃を出す（カウンターなら受付を閉じてクールタイムに入る）
	void StartRootAttack(const AttackNode& node);
	// 攻撃中にこのボタンが押されたとき、今の攻撃を切ってルート攻撃を出してよいか
	bool CanStartRootAttackOver(const PlayerStateAttack& attack, InputButton button) const;
	// 今の攻撃が、ルート攻撃で割り込める段階に来ているか（硬直が解けた・納刀モーション中）
	bool IsReadyForRootAttack(const PlayerStateAttack& attack) const;
	// 攻撃を始めたときに呼ぶ。コンボの途中でなければ新しいコンボを始める（納刀モーションは数えない）
	void BeginCombo(const std::string& attackName);
	// コンボを終える。何度呼んでもよい（終わっていれば何もしない）
	void EndCombo();

#ifdef _DEBUG
	// 選択中の攻撃が、今ルート攻撃として出せるかを並べる（出ないときに理由を探すため）
	void DrawRootAttackCheck(const std::string& attackName);
	/// <summary>
	/// 攻撃ごとのアニメーション設定（クリップ・速度・ボーン補正）。
	/// クリップ名とジョイント名は、プレイヤーのモデルが持っているものから選ばせる
	/// </summary>
	void DrawAttackAnimationEditor(const std::string& attackName);
	/// <summary>当たったときのコントローラーの振動（強さ・長さ・最終段・溜め）</summary>
	void DrawRumbleEditor(const std::string& attackName);
#endif // _DEBUG

private:
#ifdef _DEBUG
	// 攻撃エディタがこのフレームに表示した攻撃。次の Update で読み直してから空にする
	// （エディタを閉じれば読み直しも止まる）
	std::string liveEditAttack_;
#endif // _DEBUG

	std::unordered_map<std::string, AttackNode> attackGraph_;
	// ステート名とステートインスタンスのマップ
	std::unordered_map<std::string, std::unique_ptr<PlayerStateAttack>> states_;
	// 現在のステート
	std::vector<PlayerStateAttack*> currentState_;

	GlobalVariables* global_ = &GlobalVariables::GetInstance();
	// プレイヤーの参照を保持
	Player* player_ = nullptr;
	// 攻撃再生クラス
	std::unique_ptr<AttackPlayer> attackPlayer_ = nullptr;
	// コンボのリセットタイマー
	float comboResetTimer_ = 0.0f;
	// 次の攻撃入力を待っているかどうか
	bool waitingForNextCombo_ = false;
	// カウンターの受付の残り時間[秒]（実時間）
	float counterWindowTimer_ = 0.0f;
	// カウンターを出してから、次に受付が開くまでの残り時間[秒]（実時間）
	float counterCooldownTimer_ = 0.0f;
	// 攻撃の硬直中に押された、別のボタンのルート攻撃の入力。割り込めるようになったら出す
	PlayerCommand pendingRootCommand_{};
	// 上の入力を覚えておく残り時間[秒]。0 以下なら先行入力なし
	float pendingRootTimer_ = 0.0f;
	// コンボの途中か・通し番号（IsComboActive / GetComboId）
	bool comboActive_ = false;
	uint32_t comboId_ = 0;
};

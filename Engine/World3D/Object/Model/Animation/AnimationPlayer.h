#pragma once
#include <functional>
#include <string>
#include <vector>
#include <Math/Vector3.h>
#include "World3D/Object/Model/ModelStructs.h"

class AnimationClipSet;
class Skeleton;

/// <summary>
/// アニメーションの再生状態。スキンモデルのインスタンスごとに1つ持つ。
///
/// クリップ実体は AnimationClipSet（アセット側）にあり、ここは
/// 「どのクリップを、どこまで、どんなブレンドで再生しているか」だけを持つ。
/// 同じモデルを使うキャラが何体いても、それぞれ別のポーズを取れるのはこの分離のため。
///
/// ブレンドは「遷移した瞬間のポーズ」をスナップショットして、そこから新クリップへ補間する。
/// 前クリップを再サンプリングする方式だと、時刻をリセットした瞬間に姿勢が飛ぶ
/// （かつブレンド中に次のブレンドを始めると前のポーズを失う）ので採らない。
///
/// ポーズは ベースレイヤー → 上半身レイヤー → ルートモーション抽出 の順で組み立てる。
/// </summary>
class AnimationPlayer
{
public:
	// clips / skeleton はインスタンスより長生きすること
	void Initialize(const AnimationClipSet* clips, Skeleton* skeleton);

	// deltaTime[秒]。ヒットストップを効かせたいので TimeManager::GetGameDelta() を渡す想定
	void Update(float deltaTime);

	/// <summary>
	/// クリップを再生する。blendTime 秒かけて「今のポーズ」から繋ぐ。
	/// forceRestart を true にすると、同じクリップでも頭から再生し直す
	/// （コンボの同じ斬撃を連続で出す、被弾リアクションを撃ち直す、といった用途）
	/// </summary>
	void Play(const std::string& name, bool loop, float blendTime = 0.1f, bool forceRestart = false);

	// 再生速度の倍率。攻撃の緩急をつけるのに使う
	void SetSpeed(float speed) { speed_ = speed; }
	float GetSpeed() const { return speed_; }

	void SetLoop(bool loop) { loop_ = loop; }
	bool IsLoop() const { return loop_; }

	// 再生中のクリップが終端に達したか。ループ中は常に false
	bool IsFinished() const;
	// 0〜1 に正規化した再生位置
	float GetNormalizedTime() const;

	float GetTime() const { return time_; }
	/// <summary>
	/// 再生位置を直接動かす（エディタのスクラブ用）。
	/// 次の Update で飛んだ区間のイベントは発火しない
	/// </summary>
	void SetTime(float time);
	float GetDuration() const;
	const std::string& GetCurrentClipName() const { return currentName_; }
	bool IsBlending() const { return blending_; }

	// ── アニメーションイベント ──
	/// <summary>
	/// このフレームに通過したイベントのタグ。Update のたびに作り直される。
	/// 当たり判定の開始/終了やSEはここを見て出す（タイマーで二重管理しない）
	/// </summary>
	const std::vector<std::string>& GetFiredEvents() const { return firedEvents_; }
	bool WasEventFired(const std::string& tag) const;
	// 発火のたびに呼ばれる。ポーリングが面倒な場合はこちら
	void SetEventCallback(std::function<void(const std::string&)> callback) { eventCallback_ = std::move(callback); }

	// ── 上半身レイヤー ──
	/// <summary>
	/// マスクしたジョイントにだけ重ねるレイヤーを再生する。
	/// maskRootJoint とその子孫が対象（背骨のジョイント名を渡すと上半身になる）。
	/// 走りながら斬る、といった合成に使う
	/// </summary>
	void PlayLayer(const std::string& clipName, const std::string& maskRootJoint, bool loop, float blendTime = 0.1f);
	// レイヤーを blendTime 秒かけて外す
	void StopLayer(float blendTime = 0.1f);
	void SetLayerWeight(float weight) { layerTargetWeight_ = weight; }
	bool IsLayerActive() const { return layerClip_ != nullptr; }
	const std::string& GetLayerClipName() const { return layerName_; }

	// ── ルートモーション ──
	/// <summary>
	/// 指定ジョイントの水平移動をポーズから抜き取り、移動量として取り出せるようにする。
	/// 空文字を渡すと無効。踏み込みや突進をモーション通りに動かしたいときに使う
	/// </summary>
	void SetRootMotionJoint(const std::string& jointName);
	/// <summary>
	/// 前回の呼び出しからの移動量（モデルローカル）。読むと溜まっていた分は 0 に戻る。
	/// 呼び出し側はこれをキャラの向きで回してワールド移動に足す
	/// </summary>
	Vector3 ConsumeRootMotion();
	bool IsRootMotionEnabled() const { return !rootMotionJoint_.empty(); }

private:
	void FireEventsInRange(float fromTime, float toTime);
	void UpdateEvents(float previousTime, bool wrapped);
	void UpdateLayer(float deltaTime);
	void ExtractRootMotion(float previousTime, bool wrapped);
	void CacheRootMotionEndpoints();

private:
	const AnimationClipSet* clips_ = nullptr;
	Skeleton* skeleton_ = nullptr;

	const AnimationData* current_ = nullptr;
	std::string currentName_;

	float time_ = 0.0f;
	float speed_ = 1.0f;
	bool loop_ = false;

	// ブレンド
	bool blending_ = false;
	float blendTime_ = 0.0f;
	float blendTimer_ = 0.0f;
	// 遷移した瞬間のポーズ。ブレンド中はここから現在クリップへ補間する
	std::vector<QuaternionTransform> blendSourcePose_;

	// イベント
	std::vector<std::string> firedEvents_;
	std::function<void(const std::string&)> eventCallback_;

	// 上半身レイヤー
	const AnimationData* layerClip_ = nullptr;
	std::string layerName_;
	std::string layerMaskRoot_;
	std::vector<uint8_t> layerMask_;
	float layerTime_ = 0.0f;
	bool layerLoop_ = false;
	// フェードイン/アウトで実効ウェイトを目標値へ寄せる
	float layerWeight_ = 0.0f;
	float layerTargetWeight_ = 1.0f;
	float layerBlendTime_ = 0.0f;
	bool layerStopping_ = false;

	// ルートモーション
	std::string rootMotionJoint_;
	Vector3 rootMotionAccum_{ 0.0f, 0.0f, 0.0f };
	Vector3 previousRootTranslate_{ 0.0f, 0.0f, 0.0f };
	// ループ跨ぎの差分を正しく出すための、クリップ両端の値
	Vector3 rootTranslateAtStart_{ 0.0f, 0.0f, 0.0f };
	Vector3 rootTranslateAtEnd_{ 0.0f, 0.0f, 0.0f };
	bool rootMotionPrimed_ = false;
};

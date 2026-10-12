#pragma once

/// <summary>
/// チュートリアルの部屋で START（キーボードは M）の押し方を見分ける。
///
/// START は本来ポーズのボタンなので、この部屋では
/// 「短く押して離したらポーズ」「押し続けてゲージが満ちたらスキップ」に分ける。
/// 押した瞬間にポーズを開くと長押しを数えられないため、ポーズは**離した瞬間**に開く。
///
/// 入力を見るだけで、ポーズやシーン遷移は呼び出し側（GameSceneStatePlay）が行う。
/// ゲージの表示は TutorialSkipGauge が GetProgress() を読んで行う
/// </summary>
class TutorialSkipInput
{
public:
	enum class Result {
		None,  ///< 何も起きていない（押している途中も含む）
		Pause, ///< 短く押して離した。ポーズを開く
		Skip,  ///< 押し続けてゲージが満ちた。スキップする（1回だけ返す）
	};

	/// <summary>
	/// 毎フレーム呼ぶ。
	/// enabled が false の間（死亡演出中など）は押していないものとして扱い、
	/// ゲージは減っていく。その間に押されたボタンは、一度離すまで数えない
	/// </summary>
	Result Update(float deltaTime, bool enabled = true);

	/// <summary>
	/// 押しっぱなしのボタンを数えないようにする。操作に戻るたびに呼ぶ。
	/// ポーズを START で閉じたとき、その押下がそのまま長押しとして数えられるのを防ぐ
	/// </summary>
	void Reset();

	/// <summary>ゲージの進み具合 [0, 1]。1 でスキップ</summary>
	float GetProgress() const { return progress_; }

	/// <summary>今押し続けているか（ゲージを出すかの判断に使う）</summary>
	bool IsHolding() const { return holding_; }

	/// <summary>今の押下が始まってからの時間[s]</summary>
	float GetHoldTime() const { return holdTime_; }

	/// <summary>
	/// ゲージを出すか。ポーズのつもりで短く押しただけでゲージがちらつかないよう、
	/// 押し始めて kShowDelay 経ってから出す。スキップが決まったら出したまま
	/// </summary>
	bool IsGaugeVisible() const { return skipped_ || (holding_ && holdTime_ >= kShowDelay); }

	/// <summary>スキップが決まったか</summary>
	bool IsSkipped() const { return skipped_; }

	/// <summary>スキップに必要な長押しの時間[s]</summary>
	static constexpr float kHoldDuration = 1.5f;
	/// <summary>この時間より短く押して離したらポーズ扱い。それより長く押して離したら「やめた」扱いで何もしない</summary>
	static constexpr float kTapMaxDuration = 0.3f;
	/// <summary>押し始めてからゲージを出すまで[s]</summary>
	static constexpr float kShowDelay = 0.12f;
	/// <summary>離したあとゲージが減る速さ[1/s]。一瞬で消すとちらつくので少しかけて戻す</summary>
	static constexpr float kDrainSpeed = 2.5f;

private:
	/// <summary>START か M が押されているか</summary>
	static bool IsPressed();

	float progress_ = 0.0f;
	float holdTime_ = 0.0f;
	bool holding_ = false;
	bool skipped_ = false;
	/// <summary>前のフレームで押されていたか。押した瞬間を自分で見分けるのに使う</summary>
	bool prevPressed_ = true;
	/// <summary>押しっぱなしのボタンを数えない。一度離したら解除する</summary>
	bool waitRelease_ = true;
};

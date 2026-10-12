#pragma once
#include <vector>

/// <summary>1回ぶんの振動の注文</summary>
struct RumbleRequest {
	/// <summary>左モーター（低周波・重い揺れ）の強さ 0〜1</summary>
	float low = 0.0f;
	/// <summary>右モーター（高周波・鋭い揺れ）の強さ 0〜1</summary>
	float high = 0.0f;
	/// <summary>長さ[秒]（実時間）</summary>
	float duration = 0.0f;
	/// <summary>終わりぎわに弱めていく割合 0〜1（0.4 なら最後の4割で0まで下げる）</summary>
	float fadeRatio = 0.4f;
};

/// <summary>
/// ゲームパッドの振動（XInputSetState）を1か所で扱う。
///
/// - 時間は実時間で数える（ヒットストップやスローで振動が伸びないように）
/// - 同時に複数鳴っているときは、モーターごとに強い方を使う（足すと最大値に張り付く）
/// - 値が変わったときだけパッドへ送る
/// - ゲームのウィンドウが前面に無い間は止める（裏で震え続けないように）
///
/// 止め忘れると振動が鳴りっぱなしになるので、ポーズ・シーンの切り替え・終了では StopAll() を呼ぶこと
/// </summary>
class GamepadRumble
{
public:
	static GamepadRumble& GetInstance();

	/// <summary>振動を足す。強さは全体の倍率（SetMasterScale）を掛けてから送られる</summary>
	void Play(const RumbleRequest& request);

	/// <summary>毎フレーム呼ぶ（GuchisFramework::Update）。realDeltaTime は実時間の経過[秒]</summary>
	void Update(float realDeltaTime);

	/// <summary>鳴っているものをすべて止めて、パッドへ 0 を送る</summary>
	void StopAll();

	/// <summary>全体の強さ 0〜1（OPTION の VIBRATION）。0 なら何も震えない</summary>
	void SetMasterScale(float scale);
	float GetMasterScale() const { return masterScale_; }

	/// <summary>最後にパッドへ送った値 0〜1（エディタ・確認用）</summary>
	float GetCurrentLow() const { return sentLow_; }
	float GetCurrentHigh() const { return sentHigh_; }

private:
	GamepadRumble() = default;
	~GamepadRumble() = default;
	GamepadRumble(const GamepadRumble&) = delete;
	GamepadRumble& operator=(const GamepadRumble&) = delete;

	/// <summary>パッドへ送る（前回と同じなら送らない）</summary>
	void Send(float low, float high);

	struct Active {
		RumbleRequest request;
		float elapsed = 0.0f;
	};
	std::vector<Active> actives_;
	float masterScale_ = 1.0f;
	float sentLow_ = -1.0f;
	float sentHigh_ = -1.0f;
};

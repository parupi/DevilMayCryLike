#pragma once

/// <summary>
/// メニュー操作の入力をまとめる。
///
/// パッド（十字キー・左スティック）とキーボード（WASD・矢印）のどちらでも同じように扱い、
/// 押しっぱなしのリピートもここで面倒を見る。
/// 毎フレーム Update() を1回だけ呼び、その後に各問い合わせを使うこと。
/// </summary>
class MenuNavigator
{
public:
	void Update();

	bool IsUp() const { return up_.triggered; }
	bool IsDown() const { return down_.triggered; }
	bool IsLeft() const { return left_.triggered; }
	bool IsRight() const { return right_.triggered; }
	/// <summary>決定（A / SPACE / ENTER）</summary>
	bool IsDecide() const { return decide_; }
	/// <summary>取り消し（B / ESC / BACKSPACE）</summary>
	bool IsCancel() const { return cancel_; }

private:
	/// <summary>ひと方向ぶんの押下状態</summary>
	struct Axis {
		bool pressed = false;   // 今フレーム倒れているか
		bool triggered = false; // この方向へ1回ぶん動かすフレームか
		float repeatTimer = 0.0f;
	};

	static void UpdateAxis(Axis& axis, bool pressed, float deltaTime);

	Axis up_{};
	Axis down_{};
	Axis left_{};
	Axis right_{};
	bool decide_ = false;
	bool cancel_ = false;

	/// <summary>押しっぱなしでリピートが始まるまでの秒数</summary>
	static constexpr float kRepeatDelay = 0.40f;
	/// <summary>リピート中の間隔（秒）</summary>
	static constexpr float kRepeatInterval = 0.13f;
	/// <summary>スティックを倒したとみなす閾値</summary>
	static constexpr float kStickThreshold = 0.5f;
};

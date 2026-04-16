#pragma once

class Input;

// ロックオンにまつわる入力を受け渡す
class LockOnInput
{
public:
	LockOnInput() = default;
	~LockOnInput() = default;
	// 初期化
	void Initialize(Input* input);
	// 更新処理
	void Update();
	// ロックオン入力があるかを取得
	bool PushLockOnKey() const { return isPushLockOnKey_; }

private:
	// 入力
	Input* input_ = nullptr;
	// 今フレームでロックオン入力があるかを確認
	bool isPushLockOnKey_ = false;
};


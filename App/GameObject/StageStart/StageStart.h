#pragma once
class StageStart
{
public:
	StageStart() = default;
	~StageStart() = default;
	// 初期化処理
	void Initialize();
	// 更新処理
	void Update();

	bool IsComplete() const { return isComplete_; }
private:
	bool isComplete_ = false;
};


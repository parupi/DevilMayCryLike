#pragma once
#include <scene/Transition/BaseTransition.h>
#include <memory>
#include <2d/Sprite.h>
class FadeTransition : public BaseTransition
{
public:
	FadeTransition(const std::string& transitionName);
	~FadeTransition() = default;

	// 開始
	void Start(bool isFadeOut) override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;
	// 終了したかどうか
	bool IsFinished() const override { return finished_; };

private:
	float alpha_ = 0.0f;
	bool isOut_ = false;
	bool finished_ = false;
	bool isFadeOut_ = true;

	std::unique_ptr<Sprite> sprite_;
};


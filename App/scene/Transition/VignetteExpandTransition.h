#pragma once
#include <scene/Transition/BaseTransition.h>
#include <memory>
#include <offscreen/VignetteEffect.h>
class VignetteExpandTransition : public BaseTransition
{
public:
	VignetteExpandTransition(const std::string& transitionName);
	~VignetteExpandTransition() = default;

	// 初期化
	void Initialize();
	// 開始
	void Start(bool isFadeOut) override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;
	// 終了したかどうか
	bool IsFinished() const override { return finished_; };

private:
	bool isFadeOut_ = true;
	bool finished_ = false;
	float currentSoftness_ = 2.0f;
};


#pragma once
#include <Scene/Transition/BaseTransition.h>
#include <memory>
#include <Graphics/Rendering/PostEffect/VignetteEffect.h>
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
	// softness が 2.0 ⇔ 0.0 を往復するのにかかる秒数。
	// FadeTransition と同じく、フレーム数ではなく秒で進めるための基準
	static constexpr float kTransitionTime = 0.667f;

	bool isFadeOut_ = true;
	bool finished_ = false;
	float currentSoftness_ = 2.0f;
};


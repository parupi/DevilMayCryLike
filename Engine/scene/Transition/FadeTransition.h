#pragma once
#include <Scene/Transition/BaseTransition.h>
#include <memory>
#include <Graphics/Rendering/Sprite/Sprite.h>
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
	// 端から端まで暗転するのにかかる秒数。
	// 以前は「1フレームあたり0.025」で進めていたので、リフレッシュレートが上がると
	// そのぶん速く暗転してしまっていた。60fps時の見た目(40フレーム)を秒に直した値
	static constexpr float kFadeTime = 0.667f;

	float alpha_ = 0.0f;
	bool isOut_ = false;
	bool finished_ = false;
	bool isFadeOut_ = true;

	Sprite* sprite_ = nullptr;
};


#include "AnimatedSprite.h"
#include "Utility/DeltaTime.h"

AnimatedSprite::AnimatedSprite(Sprite* sprite, const GifInfo& gifInfo)
	: sprite_(sprite)
	, frameCount_(gifInfo.frameCount)
	, frameWidth_(gifInfo.frameWidth)
	, frameHeight_(gifInfo.frameHeight)
	, atlasColumns_(gifInfo.atlasColumns > 0 ? gifInfo.atlasColumns : 1)
	, frameDurations_(gifInfo.frameDurations) {
	// 表示サイズを 1 フレーム分に設定 (AdjustTextureSize がアトラス全体のサイズを設定してしまうため)
	sprite_->SetSize({static_cast<float>(frameWidth_), static_cast<float>(frameHeight_)});
	ApplyFrameUV();
}

void AnimatedSprite::Update() {
	if (!isFinished_) {
		elapsedTime_ += DeltaTime::GetDeltaTime() * speed_;

		while (elapsedTime_ >= frameDurations_[currentFrame_]) {
			elapsedTime_ -= frameDurations_[currentFrame_];
			++currentFrame_;
			if (currentFrame_ >= frameCount_) {
				if (loop_) {
					currentFrame_ = 0;
				} else {
					currentFrame_ = frameCount_ - 1;
					isFinished_ = true;
					elapsedTime_ = 0.0f;
					break;
				}
			}
		}

		ApplyFrameUV();
	}

	sprite_->Update();
}

void AnimatedSprite::Reset() {
	currentFrame_ = 0;
	elapsedTime_ = 0.0f;
	isFinished_ = false;
	ApplyFrameUV();
}

void AnimatedSprite::ApplyFrameUV() {
	int col = currentFrame_ % atlasColumns_;
	int row = currentFrame_ / atlasColumns_;
	sprite_->SetTextureLeftTop({
		static_cast<float>(col) * frameWidth_,
		static_cast<float>(row) * frameHeight_
	});
	sprite_->SetTextureSize({static_cast<float>(frameWidth_), static_cast<float>(frameHeight_)});
}

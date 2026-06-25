#pragma once
#include "Sprite.h"
#include "Graphics/Resource/GifLoader.h"

/// <summary>
/// GIF アニメーションをスプライトとして表示するクラス。
/// 内部で Sprite を保持し、フレームの UV を時間ベースで切り替える。
/// 位置・サイズ・色などは GetSprite() 経由で設定する。
/// </summary>
class AnimatedSprite {
public:
	AnimatedSprite(Sprite* sprite, const GifInfo& gifInfo);
	~AnimatedSprite() = default;

	// 毎フレーム呼ぶ。フレームを進めてから内部の Sprite::Update() を呼ぶ。
	void Update();

	// 位置・色などは Sprite に直接アクセスして設定する
	Sprite* GetSprite() const { return sprite_; }

	void SetLoop(bool loop) { loop_ = loop; }
	void SetPlaybackSpeed(float speed) { speed_ = speed; }
	bool IsFinished() const { return isFinished_; }

	// 先頭フレームに戻して再生を再開する
	void Reset();

private:
	void ApplyFrameUV();

	Sprite* sprite_ = nullptr;
	int frameCount_   = 0;
	int frameWidth_   = 0;
	int frameHeight_  = 0;
	int atlasColumns_ = 1; // GifInfo::atlasColumns と対応
	std::vector<float> frameDurations_;

	int currentFrame_ = 0;
	float elapsedTime_ = 0.0f;
	bool loop_ = true;
	float speed_ = 1.0f;
	bool isFinished_ = false;
};

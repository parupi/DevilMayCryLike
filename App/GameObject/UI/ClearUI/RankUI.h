#pragma once
#include <memory>
#include <2d/Sprite.h>
class RankUI
{
public:
	RankUI() = default;
	~RankUI() = default;

	void Initialize();

	void Update();

	void Draw();

	void Start();

private:
	std::unique_ptr<Sprite> rank_;

	bool isStart_ = false;
	float timer_ = 0.0f;

	Vector2 startPos_ = { 950.0f, -300.0f };   // 上から出現
	Vector2 targetPos_ = { 950.0f, 360.0f };   // 最終位置

	Vector2 startSize_ = { 600.0f, 600.0f };   // 大きめに登場
	Vector2 targetSize_ = { 480.0f, 480.0f };  // 本来の大きさ
};


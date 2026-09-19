#include "TutorialDecoration.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Resource/TextureManager.h"

namespace {
	// ── パネルの寸法（UI座標 1280x720）──
	// 文字は Tutorial.cpp が kTextCenterX / kCommandY / kTitleY に置くので、動かすときは一緒に見ること
	constexpr float kPanelCenterX = 210.0f;
	constexpr float kPanelCenterY = 360.0f;
	constexpr uint32_t kPanelWidth = 340;
	constexpr uint32_t kPanelHeight = 260;
	// パネルの濃さ。真っ黒にはせず、絵の側の濃淡を残す
	constexpr float kPanelAlpha = 0.85f;
	// 端が透けていく幅。左右を広く取るほど「切り取られた矩形」に見えなくなる
	constexpr float kPanelFadeX = 48.0f;
	constexpr float kPanelFadeY = 26.0f;

	// ── 飾りを置く内側の枠 ──
	// 上下の仕切り線はこの枠の辺に、四隅の飾りはこの枠の角に合わせる。
	// パネルが透け始める手前に収めること（外へ出すと飾りだけ宙に浮く）
	constexpr float kFrameLeft = 90.0f;
	constexpr float kFrameRight = 330.0f;
	constexpr float kFrameTop = 262.0f;
	constexpr float kFrameBottom = 458.0f;

	// 仕切り線。絵の飾りは縦幅の2割ほどしか使っていないので、SetSize の高さ＝線の太さではない
	constexpr float kDividerWidth = 288.0f;
	constexpr float kDividerHeight = 152.0f;

	// 四隅の飾り。タイル1枚の大きさと、線をタイルの端から内側へ入れてある量
	constexpr float kCornerSize = 38.0f;
	constexpr float kCornerInset = 3.0f;

	// 飾りの色。仕切り線の絵の生成りの白に寄せる
	constexpr float kOrnamentR = 0.92f;
	constexpr float kOrnamentG = 0.94f;
	constexpr float kOrnamentB = 1.00f;

	// 1フレームぶんのフェード量
	constexpr float kFadeSpeed = 0.01f;

	// 自前で作るテクスチャの登録名（ファイルは無いが、他のテクスチャと同じように引ける）
	const char* kPanelTextureName = "UI/Tutorial/Panel";
	const char* kCornerTextureName = "UI/Tutorial/Corner";

	// ---------------------------------------------------------------
	// ここから下は「絵のファイルを持たずに装飾を作る」ためのピクセル生成。
	// 白で作っておき、色と濃さは Sprite の SetColor で付ける（フェードがそのまま効く）
	// ---------------------------------------------------------------

	// 0→1 のなめらかな立ち上がり
	float SmoothStep01(float edge0, float edge1, float x) {
		if (edge1 <= edge0) { return (x < edge0) ? 0.0f : 1.0f; }
		const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	// 整数座標から決まる 0〜1 の乱数。毎回同じ絵になるように自前で持つ
	float Hash01(int32_t x, int32_t y, uint32_t seed) {
		uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u + seed * 2246822519u;
		h = (h ^ (h >> 13)) * 1274126177u;
		h ^= (h >> 16);
		return static_cast<float>(h & 0xFFFFFFu) / static_cast<float>(0xFFFFFF);
	}

	// 粗い格子の乱数を補間した、紙の斑のようなノイズ
	float ValueNoise(float x, float y, uint32_t seed) {
		const float fx = std::floor(x);
		const float fy = std::floor(y);
		const int32_t ix = static_cast<int32_t>(fx);
		const int32_t iy = static_cast<int32_t>(fy);
		const float tx = x - fx;
		const float ty = y - fy;
		const float ux = tx * tx * (3.0f - 2.0f * tx);
		const float uy = ty * ty * (3.0f - 2.0f * ty);
		const float lt = Hash01(ix, iy, seed);
		const float rt = Hash01(ix + 1, iy, seed);
		const float lb = Hash01(ix, iy + 1, seed);
		const float rb = Hash01(ix + 1, iy + 1, seed);
		const float top = lt + (rt - lt) * ux;
		const float bottom = lb + (rb - lb) * ux;
		return top + (bottom - top) * uy;
	}

	// 線の濃さ。距離0で1、太さの外で0
	float Stroke(float distance, float halfWidth) {
		return 1.0f - SmoothStep01(halfWidth - 0.6f, halfWidth + 0.6f, distance);
	}

	// パネル：中心が一番濃く、端へ向かって透ける。
	// 角は左右と上下のフェードの積で自然に丸くなるので、角丸の処理は要らない
	void EnsurePanelTexture() {
		// 既に登録済みなら作り直さない（シーンを出入りしても1回で済む）
		if (TextureManager::GetInstance().TryGetMetaData(kPanelTextureName) != nullptr) { return; }

		std::vector<uint8_t> pixels(static_cast<size_t>(kPanelWidth) * kPanelHeight * 4);
		const float halfW = kPanelWidth * 0.5f;
		const float halfH = kPanelHeight * 0.5f;

		for (uint32_t y = 0; y < kPanelHeight; ++y) {
			for (uint32_t x = 0; x < kPanelWidth; ++x) {
				const float px = static_cast<float>(x) + 0.5f;
				const float py = static_cast<float>(y) + 0.5f;

				// 端までの距離で透かす
				const float distX = (std::min)(px, kPanelWidth - px);
				const float distY = (std::min)(py, kPanelHeight - py);
				const float edge = SmoothStep01(0.0f, kPanelFadeX, distX) * SmoothStep01(0.0f, kPanelFadeY, distY);

				// 中心が濃いゆるい濃淡（真っ黒一色のCG感を消すため）
				const float rx = (px - halfW) / halfW;
				const float ry = (py - halfH) / halfH;
				const float radial = 1.0f - 0.12f * std::clamp(std::sqrt(rx * rx + ry * ry), 0.0f, 1.0f);
				// 紙の斑と細かい粒。強くしすぎると汚れに見えるので数%に留める
				const float blotch = (ValueNoise(px / 26.0f, py / 26.0f, 1u) - 0.5f) * 0.10f;
				const float grain = (Hash01(static_cast<int32_t>(x), static_cast<int32_t>(y), 7u) - 0.5f) * 0.05f;

				const float alpha = std::clamp(edge * (radial + blotch + grain), 0.0f, 1.0f);

				const size_t index = (static_cast<size_t>(y) * kPanelWidth + x) * 4;
				pixels[index + 0] = 255;
				pixels[index + 1] = 255;
				pixels[index + 2] = 255;
				pixels[index + 3] = static_cast<uint8_t>(alpha * 255.0f + 0.5f);
			}
		}

		TextureManager::GetInstance().LoadTextureFromMemory(kPanelTextureName, pixels.data(), kPanelWidth, kPanelHeight);
	}

	// 四隅の飾り：角を丸く回り込む線と、その円弧に重なる小さなひし形。
	// 絵は左上の角ぶんだけ作り、残りの3隅は Sprite の反転で使い回す
	void EnsureCornerTexture() {
		if (TextureManager::GetInstance().TryGetMetaData(kCornerTextureName) != nullptr) { return; }

		constexpr uint32_t kSize = static_cast<uint32_t>(kCornerSize);
		constexpr float kElbowRadius = 12.0f;   // 曲がり角の丸み
		constexpr float kLineHalfWidth = 1.2f;  // 線の太さの半分
		constexpr float kDiamondRadius = 4.2f;  // 角のひし形の大きさ
		const float cx = kCornerInset + kElbowRadius;
		const float cy = kCornerInset + kElbowRadius;
		// 円弧の真ん中（ひし形を置く位置）
		const float diamondX = cx - kElbowRadius * 0.7071f;
		const float diamondY = cy - kElbowRadius * 0.7071f;

		std::vector<uint8_t> pixels(static_cast<size_t>(kSize) * kSize * 4);

		for (uint32_t y = 0; y < kSize; ++y) {
			for (uint32_t x = 0; x < kSize; ++x) {
				const float px = static_cast<float>(x) + 0.5f;
				const float py = static_cast<float>(y) + 0.5f;

				float cover = 0.0f;
				if (px <= cx && py <= cy) {
					// 角を回り込む円弧
					const float dx = px - cx;
					const float dy = py - cy;
					cover = Stroke(std::fabs(std::sqrt(dx * dx + dy * dy) - kElbowRadius), kLineHalfWidth);
				} else if (py <= cy) {
					// 上辺へ伸びる線。仕切り線へ向かって細く消えていく
					const float t = (px - cx) / (kSize - cx);
					cover = Stroke(std::fabs(py - kCornerInset), kLineHalfWidth) * (1.0f - SmoothStep01(0.1f, 1.0f, t));
				} else if (px <= cx) {
					// 左辺へ伸びる線
					const float t = (py - cy) / (kSize - cy);
					cover = Stroke(std::fabs(px - kCornerInset), kLineHalfWidth) * (1.0f - SmoothStep01(0.1f, 1.0f, t));
				}

				// 円弧の真ん中に小さなひし形を重ねる（視線が角に留まる）
				const float diamond = std::fabs(px - diamondX) + std::fabs(py - diamondY);
				cover = (std::max)(cover, 1.0f - SmoothStep01(kDiamondRadius - 0.8f, kDiamondRadius + 0.8f, diamond));

				const size_t index = (static_cast<size_t>(y) * kSize + x) * 4;
				pixels[index + 0] = 255;
				pixels[index + 1] = 255;
				pixels[index + 2] = 255;
				pixels[index + 3] = static_cast<uint8_t>(std::clamp(cover, 0.0f, 1.0f) * 255.0f + 0.5f);
			}
		}

		TextureManager::GetInstance().LoadTextureFromMemory(kCornerTextureName, pixels.data(), kSize, kSize);
	}
}

void TutorialDecoration::Initialize() {
	// 先にテクスチャを作っておく（CreateSprite は登録済みの名前ならファイルを読みに行かない）
	EnsurePanelTexture();
	EnsureCornerTexture();

	SpriteManager& sprites = SpriteManager::GetInstance();

	// 同じレイヤーでは生成順に重なる。パネル → 仕切り線 → 四隅 →（Tutorial が作る説明文）の順
	panel_ = sprites.CreateSprite(SpriteLayer::UI, "TutorialPanel", kPanelTextureName);
	panel_->SetAnchorPoint({0.5f, 0.5f});
	panel_->SetPosition({kPanelCenterX, kPanelCenterY});
	panel_->SetSize({static_cast<float>(kPanelWidth), static_cast<float>(kPanelHeight)});
	panel_->SetColor({0.0f, 0.0f, 0.0f, 0.0f});

	// 上側の線
	underDivider_ = sprites.CreateSprite(SpriteLayer::UI, "underDivider", "UI/Menu/UnderDivider.png");
	underDivider_->SetPosition({kPanelCenterX, kFrameTop});
	underDivider_->SetSize({kDividerWidth, kDividerHeight});
	underDivider_->SetAnchorPoint({0.5f, 0.5f});
	underDivider_->SetDissolveThreshold(1.0f);

	// 下側の線。高さを負にして絵を上下反転させているのは元のまま
	upDivider_ = sprites.CreateSprite(SpriteLayer::UI, "upperDivider", "UI/Menu/UpperDivider.png");
	upDivider_->SetPosition({kPanelCenterX, kFrameBottom});
	upDivider_->SetSize({kDividerWidth, -kDividerHeight});
	upDivider_->SetAnchorPoint({0.5f, 0.5f});
	upDivider_->SetDissolveThreshold(1.0f);

	// 四隅の飾り。絵は左上ぶんしか無いので、右と下は反転で作る。
	// 反転するとアンカーの位置を基準に反対側へ伸びるので、置く座標は枠の角そのもの
	struct CornerPlacement {
		const char* name;
		float x;
		float y;
		bool flipX;
		bool flipY;
	};
	const CornerPlacement placements[kCornerCount] = {
		{ "TutorialCornerLT", kFrameLeft - kCornerInset,  kFrameTop - kCornerInset,    false, false },
		{ "TutorialCornerRT", kFrameRight + kCornerInset, kFrameTop - kCornerInset,    true,  false },
		{ "TutorialCornerLB", kFrameLeft - kCornerInset,  kFrameBottom + kCornerInset, false, true  },
		{ "TutorialCornerRB", kFrameRight + kCornerInset, kFrameBottom + kCornerInset, true,  true  },
	};
	for (int32_t i = 0; i < kCornerCount; ++i) {
		const CornerPlacement& placement = placements[i];
		Sprite* corner = sprites.CreateSprite(SpriteLayer::UI, placement.name, kCornerTextureName);
		corner->SetAnchorPoint({0.0f, 0.0f});
		corner->SetPosition({placement.x, placement.y});
		corner->SetSize({kCornerSize, kCornerSize});
		corner->SetIsFlipX(placement.flipX);
		corner->SetIsFlipY(placement.flipY);
		corner->SetColor({kOrnamentR, kOrnamentG, kOrnamentB, 0.0f});
		corners_[i] = corner;
	}

	// 始まるまでは何も出さない
	Apply();
}

void TutorialDecoration::Update() {
	switch (state_) {
	case State::Start:
		// 出具合を徐々に増やす
		progress_ += kFadeSpeed;
		if (progress_ >= 1.0f) {
			progress_ = 1.0f;
			state_ = State::Active; // アクティブな状態に遷移
		}
		break;
	case State::End:
		// 出具合を徐々に減らす
		progress_ -= kFadeSpeed;
		if (progress_ <= 0.0f) {
			progress_ = 0.0f;
			state_ = State::Inactive; // 非アクティブな状態に遷移
		}
		break;
	default:
		break;
	}

	Apply();

	panel_->Update();
	underDivider_->Update();
	upDivider_->Update();
	for (Sprite* corner : corners_) {
		corner->Update();
	}
}

void TutorialDecoration::Apply() {
	// 消えている間は描画自体を止める。
	// 仕切り線のディゾルブは 1.0 でも「ノイズ値が最大の画素だけ」残るので、これが無いと薄く残りうる
	const bool visible = progress_ > 0.0f;

	panel_->SetColor({0.0f, 0.0f, 0.0f, progress_ * kPanelAlpha});
	panel_->GetRenderState().isVisible = visible;

	// 仕切り線は今までどおりディゾルブで出し入れする
	const float threshold = 1.0f - progress_;
	underDivider_->SetDissolveThreshold(threshold);
	underDivider_->GetRenderState().isVisible = visible;
	upDivider_->SetDissolveThreshold(threshold);
	upDivider_->GetRenderState().isVisible = visible;

	for (Sprite* corner : corners_) {
		corner->SetColor({kOrnamentR, kOrnamentG, kOrnamentB, progress_});
		corner->GetRenderState().isVisible = visible;
	}
}

void TutorialDecoration::Start() {
	state_ = State::Start;
}

void TutorialDecoration::End() {
	state_ = State::End;
}

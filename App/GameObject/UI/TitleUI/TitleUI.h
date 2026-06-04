#pragma once
#include <World3D/Object/Renderer/RendererManager.h>
#include <World3D/Object/Renderer/PrimitiveRenderer.h>
#include <World3D/Object/Object3dManager.h>
#include <World3D/Object/Object3d.h>
#include <Graphics/Rendering/Sprite/Sprite.h>
#include <World3D/Object/Renderer/ModelRenderer.h>

/// <summary>
/// タイトルのUIをまとめるクラス
/// </summary>
class TitleUI
{
public:
	TitleUI() = default;
	~TitleUI() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// シーン遷移演出を始める
	/// </summary>
	void Exit();

	/// <summary>
	/// シーン遷移演出の更新
	/// </summary>
	void ExitUpdate();

private:

	// タイトルのUI群
	Sprite* titleWord_;
	Sprite* titleUnder_;
	Sprite* titleUp_;

	// セレクトのUI群
	std::array<Sprite*, 2> selectArrows_;
	Sprite* gameStart_;
	Sprite* selectMask_;

	bool isExit_ = false;
	float exitTime_ = 0.5f;
	float exitTimer_ = 0.0f;

	std::array<Vector2, 2> targetArrowSizes_;
	float targetSpriteAlpha_ = 1.0f;
	float targetSelectMaskAlpha = 0.0f;

	std::array<Vector2, 2> startArrowSizes_;
	float startSpriteAlpha_ = 1.0f;
	float startSelectMaskAlpha = 0.0f;

	Object3d* weaponObject_;
};


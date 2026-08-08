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

	// 描画は SpriteManager が UI レイヤーとして自動で行うため Draw() は持たない

	/// <summary>
	/// 「PRESS A」まわりを表示し始める。
	/// カメラの導入演出が終わってから呼ぶ想定で、それまでは出しっぱなしにしない
	/// </summary>
	void ShowPrompt();

	/// <summary>
	/// すでに操作案内を出しているかどうか
	/// </summary>
	bool IsPromptShown() const { return promptState_ != PromptState::Hidden; }

	/// <summary>
	/// シーン遷移演出を始める
	/// </summary>
	void Exit();

	/// <summary>
	/// シーン遷移演出の更新
	/// </summary>
	void ExitUpdate();

private:
	/// <summary>
	/// ロゴ・プレイヤー・剣をゆっくり動かして、止め絵に見えないようにする
	/// </summary>
	void UpdateSceneMotion(float deltaTime);

	/// <summary>
	/// 操作案内の出現と明滅
	/// </summary>
	void UpdatePrompt(float deltaTime);

private:
	/// <summary>
	/// 操作案内の状態
	/// </summary>
	enum class PromptState {
		Hidden,    // 導入演出中。まだ出さない
		Appearing, // フェードインしている最中
		Idle,      // 明滅しながら入力待ち
		Exiting,   // 決定されて消えていく最中
	} promptState_ = PromptState::Hidden;

	// セレクトのUI群
	std::array<Sprite*, 2> selectArrows_{ nullptr, nullptr };
	Sprite* gameStart_ = nullptr;
	Sprite* selectMask_ = nullptr;

	// ==========================
	// 3Dオブジェクト（漂わせるために持っておく）
	// ==========================
	Object3d* titleLogo_ = nullptr;
	Object3d* playerObject_ = nullptr;
	Object3d* weaponObject_ = nullptr;

	Vector3 logoBasePosition_{};
	Vector3 logoBaseScale_{};
	Vector3 playerBasePosition_{};
	Vector3 weaponBasePosition_{};

	/// <summary>
	/// シーンが始まってからの経過時間（漂いの位相に使う）
	/// </summary>
	float motionTimer_ = 0.0f;

	// ==========================
	// 操作案内
	// ==========================
	/// <summary>出現・明滅に使う経過時間</summary>
	float promptTimer_ = 0.0f;
	/// <summary>矢印の基準位置。明滅に合わせて左右へ寄せる基準になる</summary>
	std::array<Vector2, 2> arrowBasePositions_{};

	bool isExit_ = false;
	float exitTime_ = 0.5f;
	float exitTimer_ = 0.0f;

	std::array<Vector2, 2> targetArrowSizes_{};
	float targetSpriteAlpha_ = 1.0f;
	float targetSelectMaskAlpha = 0.0f;

	std::array<Vector2, 2> startArrowSizes_{};
	float startSpriteAlpha_ = 1.0f;
	float startSelectMaskAlpha = 0.0f;

	// ==========================
	// 演出パラメータ
	// ==========================
	/// <summary>操作案内がフェードインしきるまでの秒数</summary>
	static constexpr float kPromptAppearTime = 0.4f;
	/// <summary>明滅の速さ（ラジアン/秒）</summary>
	static constexpr float kPulseSpeed = 3.0f;
	/// <summary>明滅で暗くなりきったときのアルファ</summary>
	static constexpr float kPulseMinAlpha = 0.72f;
};

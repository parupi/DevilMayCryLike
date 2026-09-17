#pragma once
#include "Scene/BaseScene.h"
#include "Input/InputContext.h"
#include <World3D/Object/Model/Animation/BoneAttachment.h>

class BaseRenderer;
class Object3d;
class EditScene : public BaseScene
{
public:
	EditScene() = default;
	~EditScene() = default;

	// 初期化
	void Initialize() override;
	// 終了
	void Finalize() override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;

#ifdef _DEBUG
	void DebugUpdate() override;
#endif // _DEBUG

private:
	// アニメーション基盤の動作確認用。Phase3の4機能をここで動かしている。
	// 本番のゲームコードではないので、不要になったら丸ごと消してよい
	void InitializeSkinTest();
	void UpdateSkinTest();

private:
	// 入力をまとめたクラス
	std::unique_ptr<InputContext> inputContext_ = nullptr;

	// --- スキンモデル確認用 ---
	// A: 上半身レイヤー＋ボーン追従 / B: アニメーションイベント＋ルートモーション
	BaseRenderer* skinRendererA_ = nullptr;
	BaseRenderer* skinRendererB_ = nullptr;
	Object3d* skinObjectB_ = nullptr;
	Object3d* attachedSword_ = nullptr;
	std::unique_ptr<BoneAttachment> swordAttachment_;
	// イベントで光らせるための減衰値
	float eventFlash_ = 0.0f;
};


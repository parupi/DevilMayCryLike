#include "TitleUI.h"
#include <Utility/DeltaTime.h>
#include <algorithm>
#include <cmath>
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void TitleUI::Initialize() {
	RendererManager::GetInstance().AddRenderer(std::make_unique<PrimitiveRenderer>("Title", PrimitiveType::Plane, "Title.png"));

	std::unique_ptr<PrimitiveRenderer> primitive = std::make_unique<PrimitiveRenderer>("TitleUp", PrimitiveType::Plane, "TitleUp.png");
	primitive->GetWorldTransform()->GetTranslation() = {0.0f, 0.0f, 0.7f};
	RendererManager::GetInstance().AddRenderer(std::move(primitive));

	primitive = std::make_unique<PrimitiveRenderer>("TitleUnder", PrimitiveType::Plane, "TitleUnder.png");
	primitive->GetWorldTransform()->GetTranslation() = {0.0f, 0.0f, -0.8f};
	RendererManager::GetInstance().AddRenderer(std::move(primitive));

	std::unique_ptr<Object3d> object = std::make_unique<Object3d>("Title");
	object->Initialize();
	object->AddRenderer(RendererManager::GetInstance().FindRender("Title"));
	object->AddRenderer(RendererManager::GetInstance().FindRender("TitleUp"));
	object->AddRenderer(RendererManager::GetInstance().FindRender("TitleUnder"));

	object->GetOption().drawPath = DrawPath::Forward;

	object->GetRenderer("Title")->GetModel()->GetMaterials()[0]->SetIsLighting(false);
	object->GetRenderer("TitleUp")->GetModel()->GetMaterials()[0]->SetIsLighting(false);
	object->GetRenderer("TitleUnder")->GetModel()->GetMaterials()[0]->SetIsLighting(false);

	object->GetWorldTransform()->GetTranslation() = {0.0f, 3.2f, -7.0f};
	object->GetWorldTransform()->GetScale() = {6.0f, 1.0f, 2.0f};

	Vector3 dir = {-90.0f, 0.0f, 0.0f};
	object->GetWorldTransform()->GetRotation() = EulerDegree(dir);

	// 漂わせるために基準の姿勢を控えておく
	logoBasePosition_ = object->GetWorldTransform()->GetTranslation();
	logoBaseScale_ = object->GetWorldTransform()->GetScale();
	titleLogo_ = object.get();
	Object3dManager::GetInstance().AddObject(std::move(object));

	for (int32_t i = 0; i < 2; i++) {
		selectArrows_[i] = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "selectArrow" + std::to_string(i), "SelectArrow.png");
		selectArrows_[i]->SetAnchorPoint({0.5f, 0.5f});

		if (i == 0) {
			selectArrows_[i]->SetSize({32.0f, 32.0f});
			selectArrows_[i]->SetPosition({500.0f, 520.0f});
		} else {
			// x を負にして左右反転させ、内向きの矢印にしている
			selectArrows_[i]->SetSize({-32.0f, 32.0f});
			selectArrows_[i]->SetPosition({780.0f, 520.0f});
		}

		arrowBasePositions_[i] = selectArrows_[i]->GetPosition();
	}

	gameStart_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "titleUI", "TitleUI.png");
	gameStart_->SetPosition({640.0f, 520.0f});
	gameStart_->SetAnchorPoint({0.5f, 0.5f});

	selectMask_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "selectMask", "circle.png");
	selectMask_->SetPosition({640.0f, 520.0f});
	selectMask_->SetSize({500.0f, 100.0f});
	selectMask_->SetAnchorPoint({0.5f, 0.5f});
	selectMask_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	selectMask_->GetRenderState().blendMode = BlendMode::kAdd;

	// 導入のカメラ移動が終わるまでは操作案内を出さない
	gameStart_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	for (auto& arrow : selectArrows_) {
		arrow->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	}

	// プレイヤーの生成
	std::unique_ptr<Object3d> playerObject = std::make_unique<Object3d>("Player");
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>("Player", "PlayerHead"));
	playerObject->AddRenderer(RendererManager::GetInstance().FindRender("Player"));
	playerObject->GetWorldTransform()->GetTranslation() = {0.0f, 0.0f, -7.0f};
	playerBasePosition_ = playerObject->GetWorldTransform()->GetTranslation();
	playerObject_ = playerObject.get();
	Object3dManager::GetInstance().AddObject(std::move(playerObject));

	// 武器の生成
	std::unique_ptr<Object3d> weaponObject = std::make_unique<Object3d>("Sword");
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>("Sword", "Sword"));
	weaponObject->AddRenderer(RendererManager::GetInstance().FindRender("Sword"));
	weaponObject->GetWorldTransform()->GetTranslation() = {0.0f, 0.75f, -7.5f};
	weaponObject->GetWorldTransform()->GetScale() = {0.5f, 0.5f, 0.5f};
	// 回転の計算
	Vector3 direction = {135.0f, 90.0f, 0.0f};
	weaponObject->GetWorldTransform()->GetRotation() = EulerDegree(direction);

	weaponBasePosition_ = weaponObject->GetWorldTransform()->GetTranslation();
	weaponObject_ = weaponObject.get();
	Object3dManager::GetInstance().AddObject(std::move(weaponObject));
}

void TitleUI::Update() {
	const float deltaTime = DeltaTime::GetDeltaTime();

	//UpdateSceneMotion(deltaTime);
	UpdatePrompt(deltaTime);
	ExitUpdate();

	gameStart_->Update();

	for (auto& arrow : selectArrows_) {
		arrow->Update();
	}

	selectMask_->Update();
}

void TitleUI::UpdateSceneMotion(float deltaTime) {
	motionTimer_ += deltaTime;

	// ゆっくり上下させつつ、わずかに拡縮させて呼吸させる
	if (titleLogo_) {
		WorldTransform* transform = titleLogo_->GetWorldTransform();
		transform->GetTranslation().y = logoBasePosition_.y + std::sin(motionTimer_ * 0.80f) * 0.08f;

		const float breath = 1.0f + std::sin(motionTimer_ * 0.55f) * 0.010f;
		transform->GetScale() = {logoBaseScale_.x * breath, logoBaseScale_.y, logoBaseScale_.z * breath};
	}

	// 浮いているように見せる程度のごく浅い上下
	if (playerObject_) {
		playerObject_->GetWorldTransform()->GetTranslation().y = playerBasePosition_.y + std::sin(motionTimer_ * 0.60f) * 0.06f;
	}
}

void TitleUI::ShowPrompt() {
	if (promptState_ != PromptState::Hidden) return;

	promptState_ = PromptState::Appearing;
	promptTimer_ = 0.0f;
}

void TitleUI::UpdatePrompt(float deltaTime) {
	if (promptState_ == PromptState::Hidden || promptState_ == PromptState::Exiting) return;

	promptTimer_ += deltaTime;

	float alpha = 1.0f;
	if (promptState_ == PromptState::Appearing) {
		alpha = std::clamp(promptTimer_ / kPromptAppearTime, 0.0f, 1.0f);
		if (alpha >= 1.0f) {
			promptState_ = PromptState::Idle;
			promptTimer_ = 0.0f;
		}
	}

	// 0.0～1.0 を往復する明滅の係数。cos なので出た瞬間がいちばん明るい
	const float pulse = (promptState_ == PromptState::Idle)
		? 0.5f + 0.5f * std::cos(promptTimer_ * kPulseSpeed)
		: 1.0f;

	const float textAlpha = alpha * (kPulseMinAlpha + (1.0f - kPulseMinAlpha) * pulse);
	gameStart_->SetColor({1.0f, 1.0f, 1.0f, textAlpha});

	// 文字の後ろの加算グローも一緒に息をさせる
	selectMask_->SetColor({1.0f, 1.0f, 1.0f, alpha * (0.06f + 0.14f * pulse)});

	for (size_t i = 0; i < selectArrows_.size(); i++) {
		selectArrows_[i]->SetColor({1.0f, 1.0f, 1.0f, textAlpha});

		// 明滅に合わせて矢印を外へ広げ、文字を押し出しているように見せる
		const float offset = 6.0f * pulse;
		Vector2 position = arrowBasePositions_[i];
		position.x += (i == 0) ? -offset : offset;
		selectArrows_[i]->SetPosition(position);
	}
}

void TitleUI::Exit() {
	if (promptState_ == PromptState::Exiting) return;

	isExit_ = true;
	promptState_ = PromptState::Exiting;
	exitTimer_ = 0.0f;

	targetArrowSizes_[0] = {-32.0f, 32.0f};
	targetArrowSizes_[1] = {32.0f, 32.0f};
	targetSpriteAlpha_ = 0.0f;
	targetSelectMaskAlpha = 0.0f;

	for (size_t i = 0; i < selectArrows_.size(); i++) {
		startArrowSizes_[i] = selectArrows_[i]->GetSize();
		// 明滅で寄っていた位置を基準へ戻し、消えていく間は動かさない
		selectArrows_[i]->SetPosition(arrowBasePositions_[i]);
	}

	// 明滅の途中で決定されることがあるので、今の明るさから繋ぐ
	startSpriteAlpha_ = gameStart_->GetColor().w;
	startSelectMaskAlpha = selectMask_->GetColor().w;
}

void TitleUI::ExitUpdate() {
	if (!isExit_) return;

	exitTimer_ += DeltaTime::GetDeltaTime();
	// 0.0f ～ 1.0f にクランプ
	float t = std::clamp(exitTimer_ / exitTime_, 0.0f, 1.0f);

	// --- GameStart スプライトのアルファ補間 ---
	float spriteAlpha = Lerp(startSpriteAlpha_, targetSpriteAlpha_, t);
	gameStart_->SetColor({1.0f, 1.0f, 1.0f, spriteAlpha});

	// --- マスク透明度（前半フェードイン、後半フェードアウト） ---
	float maskAlpha = 0.0f;
	if (t < 0.5f) {
		float subT = t / 0.5f; // 0.0～1.0
		maskAlpha = Lerp(startSelectMaskAlpha, 0.8f, subT);
	} else {
		float subT = (t - 0.5f) / 0.5f; // 0.0～1.0
		maskAlpha = Lerp(0.8f, targetSelectMaskAlpha, subT);
	}
	selectMask_->SetColor({1.0f, 1.0f, 1.0f, maskAlpha});

	// --- 矢印のサイズ補間 ---
	for (size_t i = 0; i < selectArrows_.size(); i++) {
		Vector2 size;
		size.x = Lerp(startArrowSizes_[i].x, targetArrowSizes_[i].x, t);
		size.y = Lerp(startArrowSizes_[i].y, targetArrowSizes_[i].y, t);
		selectArrows_[i]->SetSize(size);
		selectArrows_[i]->SetColor({1.0f, 1.0f, 1.0f, spriteAlpha});
	}
}

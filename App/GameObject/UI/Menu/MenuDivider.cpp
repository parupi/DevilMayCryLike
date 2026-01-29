#include "MenuDivider.h"
#include <imgui.h>
#include <base/utility/DeltaTime.h>

void MenuDivider::Initialize()
{
	TextureManager::GetInstance()->LoadTexture("UI/Menu/UpperDivider.png");
	TextureManager::GetInstance()->LoadTexture("UI/Menu/UnderDivider.png");

	upperDivider_ = std::make_unique<Sprite>();
	upperDivider_->Initialize("UI/Menu/UpperDivider.png");
	upperDivider_->SetPosition({ 320.0f, 480.0f });
	upperDivider_->SetSize({ 640.0f, 380.0f });
	upperDivider_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

	
	underDivider_ = std::make_unique<Sprite>();
	underDivider_->Initialize("UI/Menu/UnderDivider.png");
	underDivider_->SetPosition({ 320.0f, 0.0f });
	underDivider_->SetSize({ 640.0f, 380.0f });
	underDivider_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
}

void MenuDivider::Enter()
{
	state_ = DividerState::Enter;
}

void MenuDivider::Exit()
{
	state_ = DividerState::Exit;
}

void MenuDivider::Update()
{
	//Vector2 pos = underDivider_->GetPosition();
	//Vector2 size = underDivider_->GetSize();

	//ImGui::Begin("Sprite");
	//ImGui::DragFloat2("Pos", &pos.x);
	//ImGui::DragFloat2("Size", &size.x);
	//ImGui::End();

	//underDivider_->SetPosition(pos);
	//underDivider_->SetSize(size);

	switch (state_) {
	case DividerState::Enter:
		alpha_ += DeltaTime::GetDeltaTime() * 1.5f;

		if (alpha_ > 1.0f) {
			alpha_ = 1.0f;
			state_ = DividerState::Normal;
		}
		break;
	case DividerState::Normal:

		break;
	case DividerState::Exit:
		alpha_ -= DeltaTime::GetDeltaTime() * 1.5f;

		if (alpha_ < 0.0f) {
			alpha_ = 0.0f;
			state_ = DividerState::Normal;
		}
		break;
	}

	upperDivider_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
	underDivider_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });

	upperDivider_->Update();
	underDivider_->Update();
}

void MenuDivider::Draw()
{
	upperDivider_->Draw();
	underDivider_->Draw();
}

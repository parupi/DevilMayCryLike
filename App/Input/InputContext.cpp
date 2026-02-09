#include "InputContext.h"

void InputContext::Initialize(Input* input)
{
	playerInput_ = std::make_unique<PlayerInput>();
	playerInput_->Initialize(input);
}

void InputContext::Update()
{
	if (canPlayerMove_) {
		playerInput_->Update();
	}

}

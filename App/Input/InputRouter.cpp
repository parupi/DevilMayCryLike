#include "InputRouter.h"

void InputRouter::Initialize(Input* input)
{
	playerInput_ = std::make_unique<PlayerInput>();
	playerInput_->Initialize(input);
}

void InputRouter::Update()
{
	playerInput_->Update();
}

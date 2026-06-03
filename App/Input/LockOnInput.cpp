#include "LockOnInput.h"
#include "Input/Input.h"

void LockOnInput::Initialize(Input* input)
{
	input_ = input;
}

void LockOnInput::Update()
{
	isPushLockOnKey_ = false;

	if (input_->IsConnected()) {
		if (input_->PushButton(PadNumber::ButtonR)) {
			isPushLockOnKey_ = true;
		}
	} else {
		if (input_->PushKey(DIK_P)) {
			isPushLockOnKey_ = true;
		}
	}
}

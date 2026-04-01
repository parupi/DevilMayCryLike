#include "LockOnInput.h"
#include "input/Input.h"

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
		if (input_->TriggerKey(DIK_P)) {
			isPushLockOnKey_ = true;
		}
	}
}

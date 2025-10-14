#pragma once
#include <3d/Camera/Camera.h>
class TitleCamera : public Camera
{
public:
	TitleCamera(std::string objectName);
	~TitleCamera() override = default;

	void Initialize();
	void Update() override;

	void Enter();
	void Exit();

	bool IsExit() const { return isExit_; }
private:
	enum class TitleState {
		Enter,
		Idle,
		Exit,
	}titleState_ = TitleState::Idle;

	Vector3 targetTranslate_;
	Vector3 targetRotate_;
	Vector3 startTranslate_;
	Vector3 startRotate_;

	float stateTimer_ = 0.0f;
	float stateTime_ = 5.0f;

	bool isExit_ = false;
};


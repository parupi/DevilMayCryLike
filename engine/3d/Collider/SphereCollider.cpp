#include "SphereCollider.h"
#include <Primitive/PrimitiveLineDrawer.h>
#include <cmath>

SphereCollider::SphereCollider(std::string colliderName)
{
	name = colliderName;
}

void SphereCollider::Initialize()
{
	transform_ = std::make_unique<WorldTransform>();
	transform_->Initialize();
}

void SphereCollider::Update()
{
	if (owner_ && transform_->GetParent() == nullptr) {
		transform_->SetParent(owner_->GetWorldTransform());
	}
	transform_->TransferMatrix();
}

Vector3 SphereCollider::GetCenter() const
{
	return transform_->GetWorldPos() + sphereData_.offset;
}

float SphereCollider::GetRadius() const
{
	return sphereData_.radius;
}

void SphereCollider::DrawDebug()
{
	Vector3 center = GetCenter();
	float radius = GetRadius();
	Vector4 color = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤

	PrimitiveLineDrawer* drawer = PrimitiveLineDrawer::GetInstance();

	const int segments = 20;

	// XY 平面の円
	for (int i = 0; i < segments; ++i) {
		float theta1 = (float)i / segments * 2.0f * 3.14159265f;
		float theta2 = (float)(i + 1) / segments * 2.0f * 3.14159265f;

		Vector3 p1 = {
			center.x + radius * std::cos(theta1),
			center.y + radius * std::sin(theta1),
			center.z
		};

		Vector3 p2 = {
			center.x + radius * std::cos(theta2),
			center.y + radius * std::sin(theta2),
			center.z
		};

		drawer->DrawLine(p1, p2, color);
	}

	// YZ 平面の円
	for (int i = 0; i < segments; ++i) {
		float theta1 = (float)i / segments * 2.0f * 3.14159265f;
		float theta2 = (float)(i + 1) / segments * 2.0f * 3.14159265f;

		Vector3 p1 = {
			center.x,
			center.y + radius * std::cos(theta1),
			center.z + radius * std::sin(theta1)
		};

		Vector3 p2 = {
			center.x,
			center.y + radius * std::cos(theta2),
			center.z + radius * std::sin(theta2)
		};

		drawer->DrawLine(p1, p2, color);
	}

	// ZX 平面の円
	for (int i = 0; i < segments; ++i) {
		float theta1 = (float)i / segments * 2.0f * 3.14159265f;
		float theta2 = (float)(i + 1) / segments * 2.0f * 3.14159265f;

		Vector3 p1 = {
			center.x + radius * std::sin(theta1),
			center.y,
			center.z + radius * std::cos(theta1)
		};

		Vector3 p2 = {
			center.x + radius * std::sin(theta2),
			center.y,
			center.z + radius * std::cos(theta2)
		};

		drawer->DrawLine(p1, p2, color);
	}
}

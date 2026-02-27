#include "ParticleRenderSystem.h"
#include "3d/Camera/BaseCamera.h"
#include <numbers>

void ParticleRenderSystem::BuildInstances(const std::vector<Particle>& particles, BaseCamera* camera, std::vector<InstanceData>& outInstances)
{
    outInstances.clear();
    outInstances.reserve(particles.size());

	Matrix4x4 cameraMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, camera->GetRotate(), camera->GetTranslate());
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(1280) / float(720), 0.1f, 100.0f);
	Matrix4x4 viewProjectionMatrix = viewMatrix * projectionMatrix;

	Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
	Matrix4x4 billboardMatrix = backToFrontMatrix * cameraMatrix;
	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;

    for (const auto& p : particles)
    {
        // world / wvp 計算
	    Matrix4x4 scaleMatrix = MakeScaleMatrix(p.transform.scale);
	    Matrix4x4 translateMatrix = MakeTranslateMatrix(p.transform.translate);
	    Matrix4x4 worldMatrix{};
	    if (p.isBillboard) {
		    worldMatrix = scaleMatrix * billboardMatrix * translateMatrix;
	    } else {
		    worldMatrix = MakeAffineMatrix(p.transform.scale, p.transform.rotate, p.transform.translate);
	    }

        InstanceData id{};
        id.world = worldMatrix;
        id.wvp = worldMatrix * viewProjectionMatrix;
        id.color = p.color;

        outInstances.push_back(id);
    }
}

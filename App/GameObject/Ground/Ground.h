#pragma once
#include "3d/Object/Object3d.h"
#include <base/Particle/ParticleEmitter.h>
class Ground : public Object3d
{
public:
	Ground(std::string objectName);
	~Ground() override = default;
	// 初期
	void Initialize() override;
	// 更新
	void Update(float deltaTime) override;
	// 描画
	void Draw() override;

#ifdef _DEBUG
	void DebugGui() override;
#endif // _DEBUG

private:
	std::unique_ptr<ParticleEmitter> smokeEmitter_;
	std::unique_ptr<ParticleEmitter> circleEmitter_;
};


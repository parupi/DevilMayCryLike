#pragma once
#include <vector>
#include "Particle.h"

class ParticleUpdateSystem
{
public:
	// パーティクル全体の更新
	void Update(std::vector<Particle>& particles, float dt);
private:
	// 生存時間の更新
	void UpdateLife(Particle& p, float dt);
	// Transformの更新
	void UpdateMovement(Particle& p, float dt);
	// 消滅処理の更新
	void UpdateFade(Particle& p);
};


#pragma once
#include <vector>
#include "Particle.h"
#include "ParticleGroup.h"

class ParticleUpdateSystem
{
public:
	// パーティクルグループ全体の更新
	// （params の gravity/drag と curves を参照するためグループごと受け取る）
	void Update(ParticleGroup& group, float dt);
private:
	// 生存時間の更新
	void UpdateLife(Particle& p, float dt);
	// Transformの更新（重力・空気抵抗を含む）
	void UpdateMovement(Particle& p, const ParticleParameters& params, float dt);
	// Curve / Gradient による時間変化の適用
	void UpdateCurves(Particle& p, const ParticleCurves& curves);
	// 消滅処理の更新
	// カーブが担当しているチャンネルは従来のフェード処理を行わない
	void UpdateFade(Particle& p, bool scaleDrivenByCurve, bool alphaDrivenByCurve);
};

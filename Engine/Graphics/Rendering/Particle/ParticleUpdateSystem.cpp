#define NOMINMAX
#include <algorithm>
#include <cmath>
#include "ParticleUpdateSystem.h"

void ParticleUpdateSystem::Update(ParticleGroup& group, float dt)
{
    // どのチャンネルをカーブが担当しているかを先に判定しておく。
    // カーブが設定されていないチャンネルは従来の FadeType 挙動のまま動かす（後方互換）。
    const ParticleCurves& curves = group.curves;
    const bool scaleDrivenByCurve = !curves.sizeCurve.IsEmpty();
    const bool alphaDrivenByCurve = !curves.alphaCurve.IsEmpty() || !curves.colorGradient.IsEmpty();

    for (auto& p : group.particles)
    {
        UpdateLife(p, dt);
        UpdateMovement(p, group.params, dt);
        UpdateCurves(p, curves);
        UpdateFade(p, scaleDrivenByCurve, alphaDrivenByCurve);
    }

    // 死亡削除
    group.particles.erase(
        std::remove_if(
            group.particles.begin(),
            group.particles.end(),
            [](const Particle& p)
            {
                return p.currentTime >= p.lifeTime;
            }),
        group.particles.end());
}

void ParticleUpdateSystem::UpdateLife(Particle& p, float dt)
{
    p.currentTime += dt;
}

void ParticleUpdateSystem::UpdateMovement(Particle& p, const ParticleParameters& params, float dt)
{
    // 重力（火花が落ちる・煙が昇るなど）
    p.velocity += params.gravity * dt;

    // 空気抵抗。drag は「1秒後に残る速度の割合」なので、
    // フレームレートに依存しないよう pow(drag, dt) で1フレームぶんに換算する
    if (params.drag < 1.0f) {
        const float decay = std::pow((std::max)(params.drag, 0.0f), dt);
        p.velocity *= decay;
    }

    p.transform.translate += p.velocity * dt;
}

void ParticleUpdateSystem::UpdateCurves(Particle& p, const ParticleCurves& curves)
{
    if (curves.IsEmpty()) return;

    const float t = (p.lifeTime > 0.0f)
        ? std::clamp(p.currentTime / p.lifeTime, 0.0f, 1.0f)
        : 1.0f;

    // 生成時の値を基準に毎フレーム作り直す（減算を積み重ねないので誤差が溜まらない）
    if (!curves.sizeCurve.IsEmpty()) {
        p.transform.scale = p.baseScale * curves.sizeCurve.Evaluate(t);
    }

    if (!curves.colorGradient.IsEmpty()) {
        const Vector4 g = curves.colorGradient.Evaluate(t);
        p.color = Vector4{
            g.x * p.baseColor.x,
            g.y * p.baseColor.y,
            g.z * p.baseColor.z,
            g.w * p.baseColor.w
        };
    }

    if (!curves.alphaCurve.IsEmpty()) {
        // グラデーションが色を決めている場合はその上からアルファだけ掛ける
        const float baseAlpha = curves.colorGradient.IsEmpty() ? p.baseColor.w : p.color.w;
        p.color.w = baseAlpha * curves.alphaCurve.Evaluate(t);
    }
}

void ParticleUpdateSystem::UpdateFade(Particle& p, bool scaleDrivenByCurve, bool alphaDrivenByCurve)
{
    float t = p.currentTime / p.lifeTime;
    float alpha = 1.0f;

    switch (p.fadeType) {
    case FadeType::Alpha:
        if (alphaDrivenByCurve) break;
        alpha = 1.0f - t;
        p.color.w = alpha;
        break;
    case FadeType::ScaleShrink: {
        if (scaleDrivenByCurve) break;
        const float shrinkStart = p.shrinkStart;
        if (t > shrinkStart) {
            float progress = (t - shrinkStart) / (1.0f - shrinkStart);
            float scaleFactor = std::max(0.0f, 1.0f - progress);
            p.transform.scale = p.initialScale * scaleFactor;
        } else {
            p.initialScale = p.transform.scale;
        }
        break;
    }
    default: break;
    }
}

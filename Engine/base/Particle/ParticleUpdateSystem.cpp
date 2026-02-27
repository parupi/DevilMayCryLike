#define NOMINMAX
#include <algorithm>
#include "ParticleUpdateSystem.h"

void ParticleUpdateSystem::Update(std::vector<Particle>& particles, float dt)
{
    for (auto& p : particles)
    {
        UpdateLife(p, dt);
        UpdateMovement(p, dt);
        UpdateFade(p);
    }

    // 死亡削除
    particles.erase(
        std::remove_if(
            particles.begin(),
            particles.end(),
            [](const Particle& p)
            {
                return p.currentTime >= p.lifeTime;
            }),
        particles.end());
}

void ParticleUpdateSystem::UpdateLife(Particle& p, float dt)
{
    p.currentTime += dt;
}

void ParticleUpdateSystem::UpdateMovement(Particle& p, float dt)
{
    p.transform.translate += p.velocity * dt;
}

void ParticleUpdateSystem::UpdateFade(Particle& p)
{
    float t = p.currentTime / p.lifeTime;
    float alpha = 1.0f;

    switch (p.fadeType) {
    case FadeType::Alpha:
        alpha = 1.0f - t;
        p.color.w = alpha;
        break;
    case FadeType::ScaleShrink: {
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

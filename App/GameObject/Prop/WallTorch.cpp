#include "WallTorch.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>

#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Math/MathUtils.h"
#include "Utility/Logger.h"
#include "World3D/Camera/BaseCamera.h"
#include "World3D/Light/DynamicPointLight.h"
#include "World3D/Object/Object3dManager.h"

namespace {

/// <summary>
/// -1〜1 のゆらぎ。周期の違うサインを3本重ねて、繰り返しが読めないようにしている。
/// ノイズテーブルを持たずに済むので、トーチが何本あっても増える負荷は無視できる
/// </summary>
float FlickerNoise(float t) {
	return std::sin(t * 11.3f) * 0.50f
		+ std::sin(t * 19.7f + 1.7f) * 0.32f
		+ std::sin(t * 31.1f + 4.2f) * 0.18f;
}

} // namespace

WallTorch::WallTorch(std::string objectName) : Prop(std::move(objectName)) {
	// 置いただけで灯るように既定値を入れておく。
	// ステージデータに "light" があれば SceneBuilder が Initialize() より前に上書きする
	SetLight(kDefaultLightColor, kDefaultTipOffset, kDefaultIntensity, kDefaultRadius, kDefaultDecay);
}

void WallTorch::Initialize() {
	// モデル未指定ならトーチのモデル。Prop::Initialize() は空だと "Cube" にしてしまうので先に入れる
	if (GetModelName().empty()) {
		SetModelName(kModelName);
	}

	// モデル・コライダー・ポイントライトの生成は Prop に任せる
	Prop::Initialize();

	EnsureFireVFXLoaded();
	canEmitEmber_ = ParticleManager::GetInstance().GetParticleGroups().contains(kEmberParticle);

	// トーチごとにゆらぎと発生の位相をずらす。
	// 揃っていると隣り合ったトーチが同じ瞬間に瞬いて機械仕掛けに見える
	const float phase = static_cast<float>(std::hash<std::string>{}(name_) % 997) * 0.01f;
	flickerTime_ = phase;
	flameTimer_ = std::fmod(phase, kFlameInterval);
	emberTimer_ = std::fmod(phase, kEmberInterval);
}

void WallTorch::Update(float deltaTime) {
	// ライトをトランスフォームへ追従させる（Prop の仕事）
	Prop::Update(deltaTime);

	if (!isLit_) {
		return;
	}

	// 読み込み直後などの大きな飛びで、たまったぶんの炎が一度に噴き出さないようにする
	const float dt = std::clamp(deltaTime, 0.0f, 0.1f);
	flickerTime_ += dt * kFlickerSpeed;

	const Vector3 tip = GetTipWorldPos();

	// ── ライトのゆらぎ ──
	if (DynamicPointLight* light = GetLight()) {
		light->SetIntensity(GetLightIntensity() * (1.0f + FlickerNoise(flickerTime_) * kFlickerAmount));
		// 光源自体もわずかに揺らす。壁に落ちる影が動いて「燃えている」感じが出る
		light->SetPosition(tip + Vector3{
			FlickerNoise(flickerTime_ * 0.7f + 3.1f) * kFlickerShake,
			FlickerNoise(flickerTime_ * 0.9f + 8.4f) * kFlickerShake,
			FlickerNoise(flickerTime_ * 0.8f + 5.6f) * kFlickerShake });
	}

	// ── 炎のパーティクル ──
	// 遠くのトーチまで毎フレーム粒を撒くと、本数ぶんそのまま負荷になるので切る。
	// タイマーは進めないので、近づいたときに一気に噴き出すことも無い
	if (!IsWithinEmitDistance(tip)) {
		return;
	}

	ParticleManager& particles = ParticleManager::GetInstance();

	// 炎本体は VFX のエミッター経由。1回あたりの粒数は .vfx.json 側で調整できる
	flameTimer_ += dt;
	while (flameTimer_ >= kFlameInterval) {
		flameTimer_ -= kFlameInterval;
		particles.PlayVFX(kVfxName, tip);
	}

	// 火の粉は炎よりずっとまばらなので、エミッターに束ねず自前の間隔で撒く
	// （EmitterParticle の Count は整数なので「数回に1粒」が表現できない）
	if (canEmitEmber_) {
		emberTimer_ += dt;
		while (emberTimer_ >= kEmberInterval) {
			emberTimer_ -= kEmberInterval;
			particles.Emit(kEmberParticle, tip, 1);
		}
	}
}

void WallTorch::SetLit(bool lit) {
	isLit_ = lit;

	// ライトは消さずに無効化するだけにする。
	// SetLightEnabled(false) だと Prop の hasLight_ が落ちて、
	// そのまま保存したときに色・半径などの設定ごとステージデータから消えてしまう
	if (DynamicPointLight* light = GetLight()) {
		light->SetEnabled(lit);
		if (!lit) {
			light->SetIntensity(0.0f);
		}
		// 点け直したときの明るさは次の Update() のゆらぎが入れる
	}
}

Vector3 WallTorch::GetTipWorldPos() {
	// オフセットはワールド行列に通すので、トーチを回しても縮めても穂先に付いてくる
	return Transform(GetLightOffset(), GetWorldTransform()->GetMatWorld());
}

void WallTorch::EnsureFireVFXLoaded() {
	ParticleManager& particles = ParticleManager::GetInstance();

	// パーティクルグループはシーンをまたいで残るので、未登録のときだけ読む。
	// 毎回読むと、パーティクルエディタでの調整がトーチを1本置くたびに巻き戻る
	if (particles.GetParticleGroups().contains(kFlameParticle)) {
		return;
	}

	if (!particles.LoadVFX(kVfxName)) {
		Logger::Log("WallTorch: Resource/VFX/TorchFire.vfx.json を読み込めませんでした（炎が出ません）");
	}
}

bool WallTorch::IsWithinEmitDistance(const Vector3& position) {
	BaseCamera* camera = Object3dManager::GetInstance().GetDefaultCamera();
	if (!camera) {
		return true;
	}
	return Length(camera->GetTranslate() - position) <= kEmitDistance;
}

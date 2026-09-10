#include "AttackTelegraph.h"
#include "World3D/Object/Renderer/RendererManager.h"
#include "World3D/Camera/CameraManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Graphics/Device/DirectXManager.h"
#include "Math/MathUtils.h"
#include <algorithm>
#include <cmath>

namespace {
	// ── 見た目の調整値 ──
	// マーカーの色。加算合成なので、暗いステージでも沈まない強い赤にしてある
	constexpr Vector4 kMarkerColor = { 1.0f, 0.11f, 0.07f, 1.0f };
	// 外枠の太さ[m]。ワールド単位なので、遠くの敵の予兆も同じ太さで見える
	constexpr float kOutlineWidth = 0.13f;

	// 出現フェードの長さ[s]。攻撃が始まった瞬間にいきなり出ると目に痛い
	constexpr float kAppearTime = 0.10f;
	// 判定が出てから消えるまでの長さ[s]
	constexpr float kResolveTime = 0.26f;
	// 判定が出た瞬間の閃光の減衰の速さ。大きいほど鋭く光って消える
	constexpr float kFlashDecay = 11.0f;
	// 判定が出た瞬間にマーカーを少しだけ広げる割合（弾けたように見せる）
	constexpr float kResolvePop = 0.05f;

	constexpr float kPi = 3.14159265358979323846f;
}

void AttackTelegraphParams::ApplyScale(float lateral, float forward) {
	radius *= lateral;
	halfWidth *= lateral;
	length *= forward;
	forwardOffset *= forward;
}

AttackTelegraph& AttackTelegraph::GetInstance() {
	static AttackTelegraph instance;
	return instance;
}

void AttackTelegraph::Submit(const void* ownerKey, const AttackTelegraphParams& params,
	const Vector3& groundPos, const Vector3& forward, float progress) {
	if (!enabled_) return;
	if (params.shape == TelegraphShape::None) return;

	// 水平方向の正面。真上を向いていたら向きが決まらないので出さない
	Vector3 fwd = { forward.x, 0.0f, forward.z };
	if (Length(fwd) < 0.001f) return;
	fwd = Normalize(fwd);
	// 左手座標系。正面が +Z のとき右は +X になる
	const Vector3 right = Normalize(Cross({ 0.0f, 1.0f, 0.0f }, fwd));

	Marker* marker = FindMarker(ownerKey);
	if (!marker) {
		if (markers_.size() >= kMaxMarkers) return;
		markers_.push_back(Marker{});
		marker = &markers_.back();
		marker->key = ownerKey;
	}
	// 判定が出た後に出し直されることは無い想定だが、来たら生きている扱いに戻す
	marker->resolveTimer = -1.0f;
	marker->submitted = true;
	marker->shape = params.shape;
	marker->right = right;
	marker->forward = fwd;
	marker->halfAngleRad = params.halfAngleDeg * kPi / 180.0f;
	marker->progress = std::clamp(progress, 0.0f, 1.0f);

	// 図形の中心と広さ。矩形だけは「足元から前方へ伸びる帯」なので中心が前にずれる
	Vector3 center = groundPos + fwd * params.forwardOffset;
	if (params.shape == TelegraphShape::Rect) {
		marker->extentX = params.halfWidth;
		marker->extentY = params.length * 0.5f;
		center = center + fwd * marker->extentY;
	} else {
		marker->extentX = params.radius;
		marker->extentY = params.radius;
	}
	// 地面と同じ高さだと Zファイティングで消えるので、わずかに浮かせる
	center.y += kGroundLift;
	marker->center = center;
}

void AttackTelegraph::Cancel(const void* ownerKey) {
	markers_.erase(
		std::remove_if(markers_.begin(), markers_.end(),
			[ownerKey](const Marker& m) { return m.key == ownerKey; }),
		markers_.end());
}

void AttackTelegraph::Update(float deltaTime) {
	for (Marker& m : markers_) {
		if (m.resolveTimer >= 0.0f) {
			// 判定が出た後。閃光を出しながら消えていく
			m.resolveTimer += deltaTime;
		} else if (!m.submitted) {
			// 前フレームに誰も出さなかった＝判定が出た（＝攻撃が来た）
			m.resolveTimer = 0.0f;
			m.progress = 1.0f;
		} else {
			m.appear = (std::min)(1.0f, m.appear + (kAppearTime > 0.0f ? deltaTime / kAppearTime : 1.0f));
		}
		m.submitted = false;
	}

	markers_.erase(
		std::remove_if(markers_.begin(), markers_.end(),
			[](const Marker& m) { return m.resolveTimer > kResolveTime; }),
		markers_.end());
}

void AttackTelegraph::Clear() {
	markers_.clear();
	vertexCount_ = 0;
}

void AttackTelegraph::Draw() {
	if (!enabled_ || markers_.empty()) return;

	auto* camera = CameraManager::GetInstance().GetCurrentCamera();
	if (!camera) return;

	EnsureResources();
	if (!mappedVB_ || !mappedCB_) return;

	vertexCount_ = 0;
	for (const Marker& marker : markers_) {
		if (vertexCount_ + 6 > kMaxMarkers * 6) break;
		WriteMarker(marker, mappedVB_ + vertexCount_);
		vertexCount_ += 6;
	}
	if (vertexCount_ == 0) return;

	auto& rendererMgr = RendererManager::GetInstance();
	auto* dxManager = rendererMgr.GetDxManager();
	auto* cmd = dxManager->GetCommandList();
	auto* psoManager = rendererMgr.GetPsoManager();

	mappedCB_->viewProj = camera->GetViewProjectionMatrix();

	cmd->SetPipelineState(psoManager->GetAttackMarkerPSO());
	cmd->SetGraphicsRootSignature(psoManager->GetAttackMarkerSignature());
	cmd->SetGraphicsRootConstantBufferView(0, dxManager->GetResourceManager()->GetGPUVirtualAddress(cbHandle_));
	cmd->IASetVertexBuffers(0, 1, &vbView_);
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(vertexCount_, 1, 0, 0);
}

// ─────────────────────────────────────────
// private
// ─────────────────────────────────────────

AttackTelegraph::Marker* AttackTelegraph::FindMarker(const void* ownerKey) {
	for (Marker& m : markers_) {
		if (m.key == ownerKey && m.resolveTimer < 0.0f) return &m;
	}
	return nullptr;
}

void AttackTelegraph::WriteMarker(const Marker& marker, MarkerVertex* dst) const {
	// 判定が出た後は、閃光を出しながら少しだけ広がって消える
	float alpha = marker.appear;
	float flash = 0.0f;
	float pop = 1.0f;
	if (marker.resolveTimer >= 0.0f) {
		const float t = (kResolveTime > 0.0f) ? std::clamp(marker.resolveTimer / kResolveTime, 0.0f, 1.0f) : 1.0f;
		alpha = 1.0f - t;
		flash = std::exp(-marker.resolveTimer * kFlashDecay);
		pop = 1.0f + kResolvePop * t;
	}

	const float extentX = marker.extentX * pop;
	const float extentY = marker.extentY * pop;

	const Vector4 param = {
		static_cast<float>(static_cast<int>(marker.shape)),
		marker.progress,
		marker.halfAngleRad,
		alpha
	};
	const Vector4 extent = { extentX, extentY, flash, kOutlineWidth };

	// 図形をぴったり覆う四角形。はみ出したピクセルはシェーダ側が捨てる
	const Vector2 corners[4] = {
		{ -extentX, -extentY },
		{  extentX, -extentY },
		{ -extentX,  extentY },
		{  extentX,  extentY },
	};
	MarkerVertex quad[4]{};
	for (int i = 0; i < 4; ++i) {
		quad[i].position = marker.center + marker.right * corners[i].x + marker.forward * corners[i].y;
		quad[i].local = corners[i];
		quad[i].param = param;
		quad[i].extent = extent;
		quad[i].color = kMarkerColor;
	}

	// 三角形リスト2枚（0-1-2 / 2-1-3）
	dst[0] = quad[0];
	dst[1] = quad[1];
	dst[2] = quad[2];
	dst[3] = quad[2];
	dst[4] = quad[1];
	dst[5] = quad[3];
}

void AttackTelegraph::EnsureResources() {
	if (mappedVB_ && mappedCB_) return;

	auto* dxManager = RendererManager::GetInstance().GetDxManager();
	if (!dxManager) return;
	auto* rm = dxManager->GetResourceManager();
	if (!rm) return;

	// シーンをまたいでも使い回す。数十KBなので抱えたままで構わない
	if (!mappedVB_) {
		const size_t bufSize = sizeof(MarkerVertex) * kMaxMarkers * 6;
		vbHandle_ = rm->CreateUploadBuffer(bufSize, L"AttackTelegraphVB");
		mappedVB_ = static_cast<MarkerVertex*>(rm->Map(vbHandle_));

		vbView_.BufferLocation = rm->GetGPUVirtualAddress(vbHandle_);
		vbView_.SizeInBytes = static_cast<UINT>(bufSize);
		vbView_.StrideInBytes = sizeof(MarkerVertex);
	}
	if (!mappedCB_) {
		// D3D12 の CBV は 256 バイトアラインメントが必要
		constexpr size_t cbSize = 256;
		cbHandle_ = rm->CreateUploadBuffer(cbSize, L"AttackTelegraphCB");
		mappedCB_ = static_cast<MarkerConstantData*>(rm->Map(cbHandle_));
	}
}

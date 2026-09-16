#include "WeaponTrail.h"
#include "World3D/Object/Renderer/RendererManager.h"
#include "World3D/Camera/CameraManager.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/Resource/SrvManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Graphics/Device/DirectXManager.h"
#include "Math/MathUtils.h"
#include <DirectXTex/DirectXTex.h>
#include <algorithm>
#include <cassert>
#include <cmath>

namespace {
	// 4点 p0〜p3 で p1→p2 の間を補間する（t=0 で p1、t=1 で p2）
	Vector3 CatmullRomPoint(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
		const float t2 = t * t;
		const float t3 = t2 * t;
		return (p1 * 2.0f
			+ (p2 - p0) * t
			+ (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2
			+ (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;
	}

	// 前の点とこれより近ければ積まない[m]。ヒットストップで止まっている間に同じ点で埋まって軌跡が縮むのを防ぐ
	constexpr float kMinPointDistance = 0.005f;
}

WeaponTrail::~WeaponTrail() {
	auto* rm = RendererManager::GetInstance().GetDxManager()->GetResourceManager();
	if (vbHandle_ != kInvalidBufferHandle) {
		rm->ReleaseBuffer(vbHandle_);
	}
	if (cbHandle_ != kInvalidBufferHandle) {
		rm->ReleaseBuffer(cbHandle_);
	}
}

void WeaponTrail::Initialize() {
	CreateVertexBuffer();
	CreateConstantBuffer();
	CreateTrailTexture();
}

void WeaponTrail::SetSubdivisions(uint32_t subdivisions) {
	subdivisions_ = std::clamp<uint32_t>(subdivisions, 1u, kMaxSubdivisions);
}

void WeaponTrail::Update(float deltaTime) {
	for (auto& p : points_) {
		p.age += deltaTime;
	}
	while (!points_.empty() && points_.front().age >= lifetime_) {
		points_.pop_front();
	}
}

void WeaponTrail::AddPoint(const Vector3& tip, const Vector3& hilt) {
	if (!points_.empty()) {
		const TrailPoint& last = points_.back();
		if (Length(tip - last.tip) < kMinPointDistance && Length(hilt - last.hilt) < kMinPointDistance) {
			return;
		}
	}
	if (points_.size() >= kMaxPoints) {
		points_.pop_front();
	}
	points_.push_back({tip, hilt, 0.0f});
}

void WeaponTrail::Clear() {
	points_.clear();
	vertexCount_ = 0;
}

void WeaponTrail::Draw() {
	BuildMesh();
	if (vertexCount_ < 4) return;

	auto& rendererMgr = RendererManager::GetInstance();
	auto* dxManager = rendererMgr.GetDxManager();
	auto* cmd = dxManager->GetCommandList();
	auto* psoManager = rendererMgr.GetPsoManager();
	auto* srvManager = rendererMgr.GetSrvManager();

	// カメラから ViewProjection 行列を取得して CB に書き込む
	auto* camera = CameraManager::GetInstance().GetCurrentCamera();
	if (!camera) return;
	mappedCB_->viewProj = camera->GetViewProjectionMatrix();
	mappedCB_->tintColor = tintColor_;

	// PSO・ルートシグネチャをバインド
	cmd->SetPipelineState(psoManager->GetTrailPSO(additive_));
	cmd->SetGraphicsRootSignature(psoManager->GetTrailSignature());

	// b0: 定数バッファ
	cmd->SetGraphicsRootConstantBufferView(0, dxManager->GetResourceManager()->GetGPUVirtualAddress(cbHandle_));

	// t0: テクスチャ
	srvManager->SetGraphicsRootDescriptorTable(1, textureSrvIndex_);

	// 頂点バッファ + トポロジ
	cmd->IASetVertexBuffers(0, 1, &vbView_);
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	cmd->DrawInstanced(vertexCount_, 1, 0, 0);
}

// ─────────────────────────────────────────
// private
// ─────────────────────────────────────────

void WeaponTrail::WriteVertex(uint32_t index, uint32_t total, const Vector3& tip, const Vector3& hilt, float age) {
	const float u = (total > 1) ? (static_cast<float>(index) / static_cast<float>(total - 1)) : 1.0f;
	// 古い端ほど薄く（u）、さらに寿命に近いほど薄く（振り抜いた後に残光として自然に消える）
	const float ageFade = (lifetime_ > 0.0f) ? std::clamp(1.0f - age / lifetime_, 0.0f, 1.0f) : 1.0f;
	const float alpha = u * ageFade;

	// tip 頂点 (v = 0)
	mappedVB_[index * 2 + 0] = {
		tip,
		{ u, 0.0f },
		{ 1.0f, 1.0f, 1.0f, alpha }
	};
	// hilt 頂点 (v = 1)
	mappedVB_[index * 2 + 1] = {
		hilt,
		{ u, 1.0f },
		{ 1.0f, 1.0f, 1.0f, alpha }
	};
}

void WeaponTrail::BuildMesh() {
	const uint32_t n = static_cast<uint32_t>(points_.size());
	if (n < 2) {
		vertexCount_ = 0;
		return;
	}

	// 隣り合う点の間を Catmull-Rom で分割して、速い振りでも滑らかな弧にする。
	// 端の区間は、存在しない外側の点を端の点で代用する
	const uint32_t steps = subdivisions_;
	const uint32_t total = (n - 1) * steps + 1;
	uint32_t out = 0;

	for (uint32_t i = 0; i + 1 < n; ++i) {
		const TrailPoint& p0 = points_[(i == 0) ? 0 : i - 1];
		const TrailPoint& p1 = points_[i];
		const TrailPoint& p2 = points_[i + 1];
		const TrailPoint& p3 = points_[(std::min)(i + 2, n - 1)];

		for (uint32_t s = 0; s < steps; ++s) {
			const float t = static_cast<float>(s) / static_cast<float>(steps);
			WriteVertex(out++, total,
				CatmullRomPoint(p0.tip, p1.tip, p2.tip, p3.tip, t),
				CatmullRomPoint(p0.hilt, p1.hilt, p2.hilt, p3.hilt, t),
				p1.age + (p2.age - p1.age) * t);
		}
	}
	const TrailPoint& last = points_.back();
	WriteVertex(out++, total, last.tip, last.hilt, last.age);

	vertexCount_ = out * 2;
}

void WeaponTrail::CreateVertexBuffer() {
	auto* rm = RendererManager::GetInstance().GetDxManager()->GetResourceManager();

	const size_t bufSize = sizeof(TrailVertex) * kMaxRenderPoints * 2;
	vbHandle_ = rm->CreateUploadBuffer(bufSize, L"WeaponTrailVB");
	mappedVB_ = static_cast<TrailVertex*>(rm->Map(vbHandle_));

	vbView_.BufferLocation = rm->GetGPUVirtualAddress(vbHandle_);
	vbView_.SizeInBytes = static_cast<UINT>(bufSize);
	vbView_.StrideInBytes = sizeof(TrailVertex);
}

void WeaponTrail::CreateConstantBuffer() {
	auto* rm = RendererManager::GetInstance().GetDxManager()->GetResourceManager();

	// D3D12 CBV は 256 バイトアラインメントが必要
	constexpr size_t cbSize = 256;
	cbHandle_ = rm->CreateUploadBuffer(cbSize, L"WeaponTrailCB");
	mappedCB_ = static_cast<TrailConstantData*>(rm->Map(cbHandle_));
}

void WeaponTrail::CreateTrailTexture() {
	auto& rendererMgr = RendererManager::GetInstance();
	auto* dxManager = rendererMgr.GetDxManager();
	auto* srvManager = rendererMgr.GetSrvManager();

	constexpr UINT W = 64, H = 8;

	// グラデーション画像を CPU 上で生成
	// U 方向: 0(古い端) → 1(新しい端) でアルファ増加
	// V 方向: 中央 (0.5) が最も明るく、端に向かってソフト減衰
	DirectX::ScratchImage img;
	HRESULT hr = img.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, W, H, 1, 1);
	assert(SUCCEEDED(hr));
	(void)hr; // Release では assert が消えるため明示的に未使用にする

	uint8_t* pix = img.GetPixels();
	for (UINT y = 0; y < H; y++) {
		float v = static_cast<float>(y) / (H - 1); // 0 → 1
		float vn = v * 2.0f - 1.0f; // -1 → 1
		float vFade = 1.0f - vn * vn; // 端 = 0, 中央 = 1
		vFade = std::sqrt(vFade); // ソフトカーブ

		for (UINT x = 0; x < W; x++) {
			float u = static_cast<float>(x) / (W - 1); // 0 → 1
			float alpha = u * vFade;
			UINT idx = (y * W + x) * 4;
			pix[idx + 0] = 255; // R
			pix[idx + 1] = 255; // G
			pix[idx + 2] = 255; // B
			pix[idx + 3] = static_cast<uint8_t>(alpha * 255); // A
		}
	}

	// GPU テクスチャリソースを作成 (COPY_DEST 状態で開始)
	trailTexture_ = dxManager->GetResourceFactory()->CreateTexture2D(img.GetMetadata());

	// Upload ヒープ経由でコピー。
	// UploadTextureData は内部で COPY_DEST → GENERIC_READ へバリアを張るため
	// ここで追加バリアは不要。戻り値の upload buffer は deferred release で管理。
	auto uploadBuf = dxManager->UploadTextureData(trailTexture_.Get(), img);
	dxManager->GetResourceManager()->AddPendingUpload(uploadBuf);

	// SRV 登録
	textureSrvIndex_ = srvManager->CreateSRVFromResource(trailTexture_.Get());
}

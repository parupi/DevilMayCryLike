#include "WeaponTrail.h"
#include "World3D/Object/Renderer/RendererManager.h"
#include "World3D/Camera/CameraManager.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/Resource/SrvManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Graphics/Device/DirectXManager.h"
#include "Math/MathUtils.h"
#include <DirectXTex/DirectXTex.h>
#include <cassert>
#include <cmath>

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

void WeaponTrail::Update(float deltaTime) {
	for (auto& p : points_) {
		p.age += deltaTime;
	}
	while (!points_.empty() && points_.front().age >= lifetime_) {
		points_.pop_front();
	}
}

void WeaponTrail::AddPoint(const Vector3& tip, const Vector3& hilt) {
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
	cmd->SetPipelineState(psoManager->GetTrailPSO());
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

void WeaponTrail::BuildMesh() {
	if (points_.size() < 2) {
		vertexCount_ = 0;
		return;
	}

	uint32_t n = static_cast<uint32_t>(points_.size());
	vertexCount_ = n * 2;

	for (uint32_t i = 0; i < n; i++) {
		float u = (n > 1) ? (static_cast<float>(i) / (n - 1)) : 1.0f;
		float alpha = u; // 古い端 = 0, 新しい端 = 1

		const auto& p = points_[i];

		// tip 頂点 (v = 0)
		mappedVB_[i * 2 + 0] = {
			p.tip,
			{ u, 0.0f },
			{ 1.0f, 1.0f, 1.0f, alpha }
		};
		// hilt 頂点 (v = 1)
		mappedVB_[i * 2 + 1] = {
			p.hilt,
			{ u, 1.0f },
			{ 1.0f, 1.0f, 1.0f, alpha }
		};
	}
}

void WeaponTrail::CreateVertexBuffer() {
	auto* rm = RendererManager::GetInstance().GetDxManager()->GetResourceManager();

	const size_t bufSize = sizeof(TrailVertex) * kMaxPoints * 2;
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

#include "PrimitiveLineDrawer.h"
#include <3d/Camera/CameraManager.h>
#include <numbers>
#include <base/TextureManager.h>

PrimitiveLineDrawer* PrimitiveLineDrawer::instance = nullptr;
std::once_flag PrimitiveLineDrawer::initInstanceFlag;

PrimitiveLineDrawer* PrimitiveLineDrawer::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new PrimitiveLineDrawer();
		});
	return instance;
}

void PrimitiveLineDrawer::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;
	srvManager_ = dxManager_->GetSrvManager();

	transform_ = std::make_unique<WorldTransform>();
	transform_->Initialize();

	dummyTextureIndex_ = TextureManager::GetInstance()->CreateWhiteTexture();
}

void PrimitiveLineDrawer::Finalize()
{
	// 頂点・インデックスリソースの解放
	vertexResource_.Reset();
	indexResource_.Reset();

	// ビューデータ初期化
	vertexBufferView_ = {};
	indexBufferView_ = {};

	// 頂点・インデックスのデータをクリア
	vertices_.clear();
	indices_.clear();

	// WorldTransformの破棄（unique_ptrなのでreset）
	transform_.reset();

	// 外部参照のクリア（deleteはしない）
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	srvManager_ = nullptr;

	// ダミーテクスチャのインデックスもリセット
	dummyTextureIndex_ = 0;

	delete instance;
	instance = nullptr;

	Logger::Log("PrimitiveLineDrawer finalized.\n");
}

void PrimitiveLineDrawer::BeginDraw()
{
	vertices_.clear();
	indices_.clear();
}

void PrimitiveLineDrawer::EndDraw()
{
	if (!vertices_.empty() && !indices_.empty()) {
		UpdateVertexResource();
		UpdateIndexResource();
		Draw();
	}
}

void PrimitiveLineDrawer::DrawLine(const Vector3& start, const Vector3& end, const Vector4& color)
{
	uint32_t baseIndex = static_cast<uint32_t>(vertices_.size());
	vertices_.push_back({ start, color, Vector2{0.0f, 0.0f} });
	vertices_.push_back({ end, color, Vector2{0.0f, 0.0f} });

	indices_.push_back(baseIndex);
	indices_.push_back(baseIndex + 1);
}

void PrimitiveLineDrawer::DrawWireSphere(const Vector3& center, float radius, const Vector4& color, int divide) {
	const float angleStep = 2.0f * std::numbers::pi_v<float> / divide;

	// XY平面
	for (int i = 0; i < divide; ++i) {
		float a0 = i * angleStep;
		float a1 = (i + 1) * angleStep;
		Vector3 p0 = {
			center.x + std::cos(a0) * radius,
			center.y + std::sin(a0) * radius,
			center.z
		};
		Vector3 p1 = {
			center.x + std::cos(a1) * radius,
			center.y + std::sin(a1) * radius,
			center.z
		};
		DrawLine(p0, p1, color);
	}

	// YZ平面
	for (int i = 0; i < divide; ++i) {
		float a0 = i * angleStep;
		float a1 = (i + 1) * angleStep;
		Vector3 p0 = {
			center.x,
			center.y + std::cos(a0) * radius,
			center.z + std::sin(a0) * radius
		};
		Vector3 p1 = {
			center.x,
			center.y + std::cos(a1) * radius,
			center.z + std::sin(a1) * radius
		};
		DrawLine(p0, p1, color);
	}

	// ZX平面
	for (int i = 0; i < divide; ++i) {
		float a0 = i * angleStep;
		float a1 = (i + 1) * angleStep;
		Vector3 p0 = {
			center.x + std::sin(a0) * radius,
			center.y,
			center.z + std::cos(a0) * radius
		};
		Vector3 p1 = {
			center.x + std::sin(a1) * radius,
			center.y,
			center.z + std::cos(a1) * radius
		};
		DrawLine(p0, p1, color);
	}
}


void PrimitiveLineDrawer::UpdateVertexResource()
{
	dxManager_->CreateBufferResource(sizeof(Vertex) * vertices_.size(), vertexResource_);

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * vertices_.size());
	vertexBufferView_.StrideInBytes = sizeof(Vertex);

	void* mapped = nullptr;
	vertexResource_->Map(0, nullptr, &mapped);
	memcpy(mapped, vertices_.data(), sizeof(Vertex) * vertices_.size());
	vertexResource_->Unmap(0, nullptr);
}

void PrimitiveLineDrawer::UpdateIndexResource()
{
	dxManager_->CreateBufferResource(sizeof(uint32_t) * indices_.size(), indexResource_);

	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * indices_.size());
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	void* mapped = nullptr;
	indexResource_->Map(0, nullptr, &mapped);
	memcpy(mapped, indices_.data(), sizeof(uint32_t) * indices_.size());
	indexResource_->Unmap(0, nullptr);
}

void PrimitiveLineDrawer::Draw()
{
	const Matrix4x4& viewProjection = CameraManager::GetInstance()->GetActiveCamera()->GetViewProjectionMatrix();
	Matrix4x4 worldViewProjection = transform_->GetMatWorld() * viewProjection;

	transform_->SetMapWVP(worldViewProjection);
	transform_->SetMapWorld(transform_->GetMatWorld());

	ID3D12GraphicsCommandList* cmdList = dxManager_->GetCommandList();
	cmdList->SetPipelineState(psoManager_->GetPrimitivePSO());
	cmdList->SetGraphicsRootSignature(psoManager_->GetPrimitiveSignature());
	cmdList->SetGraphicsRootConstantBufferView(0, transform_->GetConstBuffer()->GetGPUVirtualAddress());

	srvManager_->SetGraphicsRootDescriptorTable(1, dummyTextureIndex_);
	cmdList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	cmdList->IASetIndexBuffer(&indexBufferView_);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	cmdList->DrawIndexedInstanced(static_cast<UINT>(indices_.size()), 1, 0, 0, 0);
}

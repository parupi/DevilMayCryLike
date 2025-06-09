#include "PrimitiveDrawer.h"
#include <numbers>
#include <TextureManager.h>
#include <Camera/CameraManager.h>

PrimitiveDrawer* PrimitiveDrawer::instance = nullptr;
std::once_flag PrimitiveDrawer::initInstanceFlag;

PrimitiveDrawer* PrimitiveDrawer::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new PrimitiveDrawer();
		});
	return instance;
}

void PrimitiveDrawer::Initialize(DirectXManager* dxManager, PSOManager* psoManager, SrvManager* srvManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;
	srvManager_ = srvManager;

	transform_ = std::make_unique<WorldTransform>();
	transform_->Initialize();
}

void PrimitiveDrawer::Finalize()
{
	delete instance;
	instance = nullptr;
}

void PrimitiveDrawer::BeginDraw()
{
	vertices_.clear();
	indices_.clear();
}

void PrimitiveDrawer::EndDraw()
{
	if (!vertices_.empty() && !indices_.empty()) {
		UpdateVertexResource();
		UpdateIndexResource();

		UploadAndDraw(vertices_, indices_, currentTopology_);
	}
}

void PrimitiveDrawer::DrawLine(const Vector3& start, const Vector3& end, const Vector4& color)
{
	uint32_t baseIndex = static_cast<uint32_t>(vertices_.size());
	vertices_.push_back({ start, color });
	vertices_.push_back({ end, color });
	indices_.push_back(baseIndex);
	indices_.push_back(baseIndex + 1);
	currentTopology_ = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
}

void PrimitiveDrawer::DrawRing(const Vector3& center, float innerRadius, float outerRadius, Vector4 color, int kRingDivide, RingDrawMode mode, std::string filePath)
{
	bool useTexture = filePath != "";

	// テクスチャが指定されている場合は GPU ハンドルを取得
	if (useTexture) {
		textureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(filePath);
	}

	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / static_cast<float>(kRingDivide);

	for (int index = 0; index < kRingDivide; ++index) {
		float angle0 = index * radianPerDivide;
		float angle1 = (index + 1) * radianPerDivide;

		float sin0 = std::sin(angle0);
		float cos0 = std::cos(angle0);
		float sin1 = std::sin(angle1);
		float cos1 = std::cos(angle1);

		Vector3 outer0 = { center.x + cos0 * outerRadius, center.y + sin0 * outerRadius, center.z };
		Vector3 outer1 = { center.x + cos1 * outerRadius, center.y + sin1 * outerRadius, center.z };
		Vector3 inner0 = { center.x + cos0 * innerRadius, center.y + sin0 * innerRadius, center.z };
		Vector3 inner1 = { center.x + cos1 * innerRadius, center.y + sin1 * innerRadius, center.z };

		float u0 = static_cast<float>(index) / kRingDivide;
		float u1 = static_cast<float>(index + 1) / kRingDivide;

		// UVは内外でv=1.0と0.0に固定
		Vector2 uvOuter0 = { u0, 0.0f };
		Vector2 uvOuter1 = { u1, 0.0f };
		Vector2 uvInner0 = { u0, 1.0f };
		Vector2 uvInner1 = { u1, 1.0f };

		uint32_t baseIndex = static_cast<uint32_t>(vertices_.size());

		// 頂点追加
		vertices_.push_back({ outer0, color, useTexture ? uvOuter0 : Vector2{} });
		vertices_.push_back({ outer1, color, useTexture ? uvOuter1 : Vector2{} });
		vertices_.push_back({ inner0, color, useTexture ? uvInner0 : Vector2{} });
		vertices_.push_back({ inner1, color, useTexture ? uvInner1 : Vector2{} });

		if (mode == RingDrawMode::Line) {
			indices_.push_back(baseIndex + 0); indices_.push_back(baseIndex + 1); // outer
			indices_.push_back(baseIndex + 2); indices_.push_back(baseIndex + 3); // inner
			indices_.push_back(baseIndex + 0); indices_.push_back(baseIndex + 2); // side
			indices_.push_back(baseIndex + 1); indices_.push_back(baseIndex + 3); // side
			currentTopology_ = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		} else {
			indices_.push_back(baseIndex + 0); indices_.push_back(baseIndex + 1); indices_.push_back(baseIndex + 2);
			indices_.push_back(baseIndex + 1); indices_.push_back(baseIndex + 3); indices_.push_back(baseIndex + 2);
			currentTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		}
	}
}

void PrimitiveDrawer::DrawCylinder(const Vector3& center, float divide, float topRadius, float bottomRadius, float height, std::string filePath)
{
	bool useTexture = filePath != "";

	// テクスチャが指定されている場合は GPU ハンドルを取得
	if (useTexture) {
		textureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(filePath);
	}

	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / static_cast<float>(divide);

	for (uint32_t index = 0; index < divide; ++index) {
		float sin0 = std::sin(index * radianPerDivide);
		float cos0 = std::cos(index * radianPerDivide);
		float sin1 = std::sin((index + 1) * radianPerDivide);
		float cos1 = std::cos((index + 1) * radianPerDivide);
		float u0 = float(index) / divide;
		float u1 = float(index + 1) / divide;

		// UVは内外でv=1.0と0.0に固定
		Vector2 uvOuter0 = { u0, 0.0f };
		Vector2 uvOuter1 = { u1, 0.0f };
		Vector2 uvInner0 = { u0, 1.0f };
		Vector2 uvInner1 = { u1, 1.0f };

		// 頂点4つ
		Vertex v0 = { { center.x + -sin0 * topRadius, center.y + height, center.z + cos0 * topRadius },{ 1.0f, 1.0f, 1.0f, 1.0f } ,useTexture ? uvOuter0 : Vector2{} };
		Vertex v1 = { { center.x + -sin0 * bottomRadius, center.y + 0.0f, center.z + cos0 * bottomRadius }, { 1.0f, 1.0f, 1.0f, 1.0f } ,useTexture ? uvInner0 : Vector2{} };
		Vertex v2 = { { center.x + -sin1 * topRadius, center.y + height, center.z + cos1 * topRadius }, { 1.0f, 1.0f, 1.0f, 1.0f },useTexture ? uvOuter1 : Vector2{} };
		Vertex v3 = { { center.x + -sin1 * bottomRadius, center.y + 0.0f, center.z + cos1 * bottomRadius },{ 1.0f, 1.0f, 1.0f, 1.0f } ,useTexture ? uvInner1 : Vector2{} };

		uint16_t baseIndex = static_cast<uint16_t>(vertices_.size());
		vertices_.push_back(v0);
		vertices_.push_back(v1);
		vertices_.push_back(v2);
		vertices_.push_back(v3);

		// 三角形2枚
		indices_.push_back(baseIndex + 0);
		indices_.push_back(baseIndex + 1);
		indices_.push_back(baseIndex + 2);

		indices_.push_back(baseIndex + 2);
		indices_.push_back(baseIndex + 1);
		indices_.push_back(baseIndex + 3);
		currentTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
}

void PrimitiveDrawer::UpdateVertexResource()
{
	vertexResource_ = dxManager_->CreateBufferResource(sizeof(Vertex) * vertices_.size());

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * vertices_.size());
	vertexBufferView_.StrideInBytes = sizeof(Vertex);

	void* mapped = nullptr;
	vertexResource_->Map(0, nullptr, &mapped);
	memcpy(mapped, vertices_.data(), sizeof(Vertex) * vertices_.size());
	vertexResource_->Unmap(0, nullptr);
}

void PrimitiveDrawer::UpdateIndexResource()
{
	indexResource_ = dxManager_->CreateBufferResource(sizeof(uint32_t) * indices_.size());

	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * indices_.size());
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	void* mapped = nullptr;
	indexResource_->Map(0, nullptr, &mapped);
	memcpy(mapped, indices_.data(), sizeof(uint32_t) * indices_.size());
	indexResource_->Unmap(0, nullptr);
}

void PrimitiveDrawer::UploadAndDraw(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, D3D_PRIMITIVE_TOPOLOGY topology)
{
	const Matrix4x4& viewProjectionMatrix = CameraManager::GetInstance()->GetActiveCamera()->GetViewProjectionMatrix();
	Matrix4x4 worldViewProjectionMatrix = transform_->GetMatWorld() * viewProjectionMatrix;

	transform_->SetMapWVP(worldViewProjectionMatrix);
	transform_->SetMapWorld(transform_->GetMatWorld());

	// 頂点バッファをアップロード
   // インデックスバッファをアップロード
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetPrimitivePSO().Get());
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetPrimitiveSignature().Get());
	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, transform_->GetConstBuffer()->GetGPUVirtualAddress());
	srvManager_->SetGraphicsRootDescriptorTable(1, textureIndex_);
	dxManager_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	dxManager_->GetCommandList()->IASetIndexBuffer(&indexBufferView_);
	dxManager_->GetCommandList()->IASetPrimitiveTopology(topology);
	dxManager_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);
	//dxManager_->GetCommandList()->DrawInstanced(static_cast<UINT>(vertices.size()), 1, 0, 0);
}

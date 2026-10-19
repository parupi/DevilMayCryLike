#include "PrimitiveLineDrawer.h"
#include <World3D/Camera/CameraManager.h>
#include <numbers>
#include "Graphics/Resource/TextureManager.h"

PrimitiveLineDrawer& PrimitiveLineDrawer::GetInstance()
{
	static PrimitiveLineDrawer instance;
	return instance;
}

void PrimitiveLineDrawer::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;
	srvManager_ = dxManager_->GetSrvManager();

	CreateVertexResource(dxManager);
	CreateIndexResource(dxManager);

	transform_ = std::make_unique<WorldTransform>();
	transform_->Initialize();
}

void PrimitiveLineDrawer::Finalize()
{
	// 頂点・インデックスリソースの解放
	if (dxManager_) {
		auto* resourceManager = dxManager_->GetResourceManager();
		resourceManager->ReleaseBuffer(vertexHandle_);
		resourceManager->ReleaseBuffer(indexHandle_);
	}
	vertexHandle_ = kInvalidBufferHandle;
	indexHandle_ = kInvalidBufferHandle;

	// ビューデータ初期化
	vertexBufferView_ = {};
	indexBufferView_ = {};

	// WorldTransformの破棄（unique_ptrなのでreset）
	transform_.reset();

	// 外部参照のクリア（deleteはしない）
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	srvManager_ = nullptr;


	Logger::Log("PrimitiveLineDrawer finalized.\n");
}

void PrimitiveLineDrawer::CreateVertexResource(DirectXManager* dxManager)
{
	// --- Vertex buffer ---
	// ResourceManager 経由（Map が返すのは CPU 側の写しで、提出の直前に GPU へ写される）。
	// 直接 Map した upload ヒープへ書くと、CPU と GPU が並行して走るときに GPU が読んでいる最中に書き換えてしまう
	auto* resourceManager = dxManager->GetResourceManager();
	vertexHandle_ = resourceManager->CreateUploadBuffer(sizeof(Vertex) * kMaxLineVertices, L"PrimitiveLineVB", ResourceManager::UploadUsage::Ranged);
	mappedVertexPtr_ = reinterpret_cast<Vertex*>(resourceManager->Map(vertexHandle_));

	vertexBufferView_.BufferLocation = resourceManager->GetGPUVirtualAddress(vertexHandle_);
	vertexBufferView_.SizeInBytes = sizeof(Vertex) * kMaxLineVertices;
	vertexBufferView_.StrideInBytes = sizeof(Vertex);
}

void PrimitiveLineDrawer::CreateIndexResource(DirectXManager* dxManager)
{
	auto* resourceManager = dxManager->GetResourceManager();
	indexHandle_ = resourceManager->CreateUploadBuffer(sizeof(uint32_t) * kMaxLineIndices, L"PrimitiveLineIB", ResourceManager::UploadUsage::Ranged);
	mappedIndexPtr_ = reinterpret_cast<uint32_t*>(resourceManager->Map(indexHandle_));

	indexBufferView_.BufferLocation = resourceManager->GetGPUVirtualAddress(indexHandle_);
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * kMaxLineIndices;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
}

void PrimitiveLineDrawer::BeginDraw()
{
	currentVertexCount_ = 0;
	currentIndexCount_ = 0;
}

void PrimitiveLineDrawer::EndDraw()
{
	if (currentIndexCount_ == 0) return;

	Draw();
}

void PrimitiveLineDrawer::DrawLine(const Vector3& start, const Vector3& end, const Vector4& color)
{
	if (currentVertexCount_ + 2 >= kMaxLineVertices) return;
	if (currentIndexCount_ + 2 >= kMaxLineIndices) return;

	uint32_t baseIndex = currentVertexCount_;

	mappedVertexPtr_[currentVertexCount_++] = { start, color, {0,0} };
	mappedVertexPtr_[currentVertexCount_++] = { end,   color, {0,0} };

	mappedIndexPtr_[currentIndexCount_++] = baseIndex;
	mappedIndexPtr_[currentIndexCount_++] = baseIndex + 1;
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

void PrimitiveLineDrawer::DrawWireCircle(const Vector3& center, float radius, const Vector3& normal, const Vector4& color, int divide)
{
	if (radius <= 0.0f) return;

	const float angleStep = 2.0f * std::numbers::pi_v<float> / divide;

	// 法線正規化
	Vector3 n = Normalize(normal);

	// 法線に直交するベクトルを作る
	Vector3 tangent;

	if (std::abs(n.y) < 0.99f)
	{
		tangent = Normalize(Cross(n, { 0,1,0 }));
	} else
	{
		tangent = Normalize(Cross(n, { 1,0,0 }));
	}

	Vector3 bitangent = Cross(n, tangent);

	for (int i = 0; i < divide; ++i)
	{
		float a0 = i * angleStep;
		float a1 = (i + 1) * angleStep;

		Vector3 p0 =
			center +
			tangent * std::cos(a0) * radius +
			bitangent * std::sin(a0) * radius;

		Vector3 p1 =
			center +
			tangent * std::cos(a1) * radius +
			bitangent * std::sin(a1) * radius;

		DrawLine(p0, p1, color);
	}
}

void PrimitiveLineDrawer::Draw()
{
	// ビュープロジェクション行列を送る準備
	const Matrix4x4& viewProjection = CameraManager::GetInstance().GetActiveCamera()->GetViewProjectionMatrix();
	transform_->SetMatWVP(viewProjection);

	ID3D12GraphicsCommandList* cmdList = dxManager_->GetCommandList();

	cmdList->SetPipelineState(psoManager_->GetPrimitivePSO());
	cmdList->SetGraphicsRootSignature(psoManager_->GetPrimitiveSignature());
	transform_->BindToShader(cmdList, 0);

	cmdList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	cmdList->IASetIndexBuffer(&indexBufferView_);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	// 積んだ本数ぶんだけ GPU へ写してもらう（バッファは 2万頂点ぶんあるので全部は写さない）
	auto* resourceManager = dxManager_->GetResourceManager();
	resourceManager->MarkDirty(vertexHandle_, 0, sizeof(Vertex) * currentVertexCount_);
	resourceManager->MarkDirty(indexHandle_, 0, sizeof(uint32_t) * currentIndexCount_);

	cmdList->DrawIndexedInstanced(currentIndexCount_, 1, 0, 0, 0);
}

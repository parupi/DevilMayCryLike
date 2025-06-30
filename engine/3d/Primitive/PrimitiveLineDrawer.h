#pragma once
#include <DirectXManager.h>
#include <PSOManager.h>
#include <Vector3.h>
#include <Vector4.h>
#include <WorldTransform.h>
#include <vector>
#include <memory>
#include <mutex>
#include <Vector2.h>

class PrimitiveLineDrawer
{
private:
	static PrimitiveLineDrawer* instance;
	static std::once_flag initInstanceFlag;

	PrimitiveLineDrawer() = default;
	~PrimitiveLineDrawer() = default;
	PrimitiveLineDrawer(const PrimitiveLineDrawer&) = delete;
	PrimitiveLineDrawer& operator=(const PrimitiveLineDrawer&) = delete;

public:
	static PrimitiveLineDrawer* GetInstance();

	void Initialize(DirectXManager* dxManager, PSOManager* psoManager, SrvManager* srvManager);
	void Finalize();

	void BeginDraw();
	void EndDraw();

	void DrawLine(const Vector3& start, const Vector3& end, const Vector4& color);
	void DrawWireSphere(const Vector3& center, float radius, const Vector4& color, int divide = 16);

private:
	struct Vertex {
		Vector3 position;
		Vector4 color;
		Vector2 texcoord; // ← 必須
	};

	void UpdateVertexResource();
	void UpdateIndexResource();
	void Draw();

private:
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	std::unique_ptr<WorldTransform> transform_;
	std::vector<Vertex> vertices_;
	std::vector<uint32_t> indices_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	uint32_t dummyTextureIndex_ = 0;
};

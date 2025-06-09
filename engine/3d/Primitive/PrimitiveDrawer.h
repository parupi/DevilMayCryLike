#pragma once
#include <DirectXManager.h>
#include <PSOManager.h>
#include <Vector3.h>
#include <WorldTransform.h>
#include <Vector2.h>

enum class RingDrawMode {
	Line,       // 輪郭線のみ
	Fill        // 面（TRIANGLELIST）
};

class PrimitiveDrawer
{
private:
	static PrimitiveDrawer* instance;
	static std::once_flag initInstanceFlag;

	PrimitiveDrawer() = default;
	~PrimitiveDrawer() = default;
	PrimitiveDrawer(PrimitiveDrawer&) = default;
	PrimitiveDrawer& operator=(PrimitiveDrawer&) = default;
public:
	// シングルトンインスタンスの取得
	static PrimitiveDrawer* GetInstance();
	// 初期化
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager, SrvManager* srvManager);
	// 終了
	void Finalize();

	// 描画前処理
	void BeginDraw();
	// 描画後処理
	void EndDraw();

public:


	struct Vertex {
		Vector3 position;
		Vector4 color;
		Vector2 texcoord;
	};

public:
	// 線描画
	void DrawLine(const Vector3& start, const Vector3& end, const Vector4& color);
	// リング描画
	void DrawRing(const Vector3& center, float innerRadius, float outerRadius, Vector4 color, int kRingDivide, RingDrawMode mode = RingDrawMode::Fill, std::string filePath = "");
	// シリンダー描画
	void DrawCylinder(const Vector3& center, float divide, float topRadius, float bottomRadius, float height, std::string filePath = "");
private:
	// リソースの更新
	void UpdateVertexResource();
	void UpdateIndexResource();
	// リングの描画
	void  UploadAndDraw(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, D3D_PRIMITIVE_TOPOLOGY topology);

private:
	DirectXManager* dxManager_;
	PSOManager* psoManager_;
	SrvManager* srvManager_;
	std::unique_ptr<WorldTransform> transform_;

	D3D_PRIMITIVE_TOPOLOGY currentTopology_ = D3D_PRIMITIVE_TOPOLOGY_LINELIST;

	std::vector<Vertex> vertices_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	std::vector<uint32_t> indices_;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	uint32_t textureIndex_;
};


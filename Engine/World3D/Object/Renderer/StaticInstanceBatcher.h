#pragma once
#include <cstdint>
#include <vector>
#include "Math/Matrix4x4.h"
#include "Graphics/Resource/ResourceManager.h"

class DirectXManager;
class PSOManager;
class ModelRenderer;
class Model;

/// <summary>
/// 動かない同じモデル（ステージの床・壁・岩）を、モデルごとに1回の描画命令へまとめる。
///
/// ModelRenderer::SetInstanced(true) にしたものが対象（いまは Ground が立てる）。
/// GBuffer・影の各パスで、カリングを通ったものを Submit で集め、パスの最後の Flush で
/// 「インスタンスの行列を StructuredBuffer へ書く → モデルのメッシュごとに DrawIndexedInstanced」する。
/// 行列は毎フレーム1つのアップロードバッファへ前から詰める（BeginFrame で先頭へ戻す）。
///
/// ・カメラの VP はルート定数で渡すので、まとめて描くレンダラーはオブジェクトごとの WVP を更新しない
///   （ModelRenderer::Update）。
/// ・ディゾルブ中・発光中のものは見た目がレンダラーごとに違うので、まとめずに今までどおり1個ずつ描く。
/// ・アップロードバッファは1枚。CPU と GPU が並行して走るようにする（軽量化 Step4）なら2重化すること。
/// </summary>
class StaticInstanceBatcher
{
public:
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager);

	// フレームの最初（影のパスより前）に1回。書き込み位置を先頭へ戻す
	void BeginFrame();

	// まとめて描くかを決める窓口。無効ならレンダラーは今までどおり1個ずつ描く
	void SetEnabled(bool enabled) { enabled_ = enabled; }
	bool IsEnabled() const { return enabled_; }

	// このパスで描くものとして登録する（カリングは呼ぶ側で済ませておく）
	void SubmitGBuffer(ModelRenderer* renderer, Model* model, bool environmentSurface);
	void SubmitShadow(ModelRenderer* renderer, Model* model);

	// 登録したものをまとめて描く。描き終えたら通常の PSO・ルートシグネチャに戻す
	void FlushGBuffer(const Matrix4x4& viewProj);
	void FlushShadow(const Matrix4x4& lightViewProj);
	// 描けない（カメラが無い）ときに、登録だけ捨てる。残すと次のフレームへ持ち越して溜まる
	void DiscardGBuffer() { for (Group& group : gbufferGroups_) group.items.clear(); }

	// 直近のフレームの集計（Render ウィンドウ用）
	uint32_t GetLastGBufferInstances() const { return lastGBufferInstances_; }
	uint32_t GetLastGBufferDraws() const { return lastGBufferDraws_; }
	uint32_t GetLastShadowInstances() const { return lastShadowInstances_; }
	uint32_t GetLastShadowDraws() const { return lastShadowDraws_; }

private:
	// HLSL の InstanceTransform と並びを合わせること（GBufferInstanced.VS / CascadedShadowMapInstanced.VS）
	struct InstanceTransform {
		Matrix4x4 world;
		Matrix4x4 worldInverseTranspose;
	};

	struct Group {
		Model* model = nullptr;
		bool environmentSurface = false;
		std::vector<ModelRenderer*> items;
	};

	static Group& FindGroup(std::vector<Group>& groups, Model* model, bool environmentSurface);
	// グループの行列をバッファへ書き、先頭の GPU アドレスを返す。入りきらなければ 0
	D3D12_GPU_VIRTUAL_ADDRESS WriteInstances(const Group& group);

	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;

	bool enabled_ = true;

	// 1フレームに書ける行列の数。いまのステージで GBuffer + 影4段 = 1000 程度
	static constexpr uint32_t kCapacity = 16384;
	BufferHandle instanceBuffer_ = kInvalidBufferHandle;
	InstanceTransform* mapped_ = nullptr;
	uint32_t cursor_ = 0;
	bool overflowLogged_ = false;

	std::vector<Group> gbufferGroups_;
	std::vector<Group> shadowGroups_;

	// 集計（BeginFrame で前フレームの値として確定する）
	uint32_t gbufferInstances_ = 0;
	uint32_t gbufferDraws_ = 0;
	uint32_t shadowInstances_ = 0;
	uint32_t shadowDraws_ = 0;
	uint32_t lastGBufferInstances_ = 0;
	uint32_t lastGBufferDraws_ = 0;
	uint32_t lastShadowInstances_ = 0;
	uint32_t lastShadowDraws_ = 0;
};

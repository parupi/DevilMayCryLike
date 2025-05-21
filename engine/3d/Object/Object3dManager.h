#pragma once
#include "DirectXManager.h"
#include <Camera.h>
#include <memory>
#include <mutex>
#include "PSOManager.h"
class Object3dManager
{
private:
	static Object3dManager* instance;
	static std::once_flag initInstanceFlag;

	Object3dManager() = default;
	~Object3dManager() = default;
	Object3dManager(Object3dManager&) = default;
	Object3dManager& operator=(Object3dManager&) = default;
public:
	// シングルトンインスタンスの取得
	static Object3dManager* GetInstance();
	// 初期化
	void Initialize(DirectXManager* directXManager, PSOManager* psoManager);
	// 終了
	void Finalize();
	// 描画前処理
	void DrawSet();
	// アニメーション用描画前処理
	void DrawSetForAnimation();

private:
	// DirectXのポインタ
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	// カメラのポインタ
	Camera* defaultCamera_ = nullptr;

	// ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureForAnimation_ = nullptr;
	// PSO
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStateForAnimation_ = nullptr;

	std::array<D3D12_INPUT_ELEMENT_DESC, 3> inputElementDescs_;
	std::array<D3D12_INPUT_ELEMENT_DESC, 5> inputElementDescsForAnimation_;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDescForAnimation_{};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc_{};
	// BlendState
	D3D12_BLEND_DESC blendDesc_{};
	D3D12_RASTERIZER_DESC rasterizerDesc_{};

	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlobForAnimation_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_ = nullptr;

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* signatureBlobForAnimation_ = nullptr;
	ID3DBlob* errorBlob = nullptr;
	ID3DBlob* errorBlobForAnimation_ = nullptr;

public: // ゲッター // セッター //
	DirectXManager* GetDxManager() const { return dxManager_; }
	// デフォルトカメラ
	void SetDefaultCamera(Camera* camera) { defaultCamera_ = camera; }
	Camera* GetDefaultCamera() const { return defaultCamera_; }
};


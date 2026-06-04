#include "FinalCompositePipeline.h"
#include "Graphics/Device/DirectXManager.h"
#include <cassert>
#include <DirectXTex/d3dx12.h>

Microsoft::WRL::ComPtr<ID3D12RootSignature> FinalCompositePipeline::CreateRootSignature(DirectXManager* dxManager)
{
	CD3DX12_DESCRIPTOR_RANGE rangeSRV;
	rangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

	CD3DX12_ROOT_PARAMETER params[1];
	params[0].InitAsDescriptorTable(1, &rangeSRV, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC staticSamplerDesc(
		0, // s0
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init(
		_countof(params), params,
		1, &staticSamplerDesc,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ID3DBlob* signatureBlob;
	ID3DBlob* errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	hr = dxManager->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));
	return rootSignature;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> FinalCompositePipeline::CreatePSO(
	DirectXManager* dxManager, ID3D12RootSignature* rootSignature, bool isPostCopy)
{
	auto vertexShaderBlob = dxManager->CompileShader(L"./resource/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
	auto pixelShaderBlob = dxManager->CompileShader(L"./resource/shaders/FinalComposite.PS.hlsl", L"ps_6_0");
	assert(vertexShaderBlob && pixelShaderBlob);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.pRootSignature = rootSignature;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = isPostCopy ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = dxManager->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	assert(SUCCEEDED(hr));
	return pso;
}

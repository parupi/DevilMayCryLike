#include "CompositePipeline.h"
#include "Graphics/Device/DirectXManager.h"
#include <cassert>
#include <DirectXTex/d3dx12.h>

Microsoft::WRL::ComPtr<ID3D12RootSignature> CompositePipeline::CreateRootSignature(DirectXManager* dxManager)
{
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0; // s0
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Deferred SRV (t0)
	D3D12_DESCRIPTOR_RANGE descriptorRangesForDeferredSrv[1] = {};
	descriptorRangesForDeferredSrv[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangesForDeferredSrv[0].NumDescriptors = 1;
	descriptorRangesForDeferredSrv[0].BaseShaderRegister = 0;
	descriptorRangesForDeferredSrv[0].RegisterSpace = 0;
	descriptorRangesForDeferredSrv[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// Deferred DSV (t1)
	D3D12_DESCRIPTOR_RANGE descriptorRangesForDeferredDsv[1] = {};
	descriptorRangesForDeferredDsv[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangesForDeferredDsv[0].NumDescriptors = 1;
	descriptorRangesForDeferredDsv[0].BaseShaderRegister = 1;
	descriptorRangesForDeferredDsv[0].RegisterSpace = 0;
	descriptorRangesForDeferredDsv[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// Forward SRV (t2)
	D3D12_DESCRIPTOR_RANGE descriptorRangesForForwardSrv[1] = {};
	descriptorRangesForForwardSrv[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangesForForwardSrv[0].NumDescriptors = 1;
	descriptorRangesForForwardSrv[0].BaseShaderRegister = 2;
	descriptorRangesForForwardSrv[0].RegisterSpace = 0;
	descriptorRangesForForwardSrv[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// Forward DSV (t3)
	D3D12_DESCRIPTOR_RANGE descriptorRangesForForwardDsv[1] = {};
	descriptorRangesForForwardDsv[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangesForForwardDsv[0].NumDescriptors = 1;
	descriptorRangesForForwardDsv[0].BaseShaderRegister = 3;
	descriptorRangesForForwardDsv[0].RegisterSpace = 0;
	descriptorRangesForForwardDsv[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangesForDeferredSrv);
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRangesForDeferredSrv;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangesForDeferredDsv);
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangesForDeferredDsv;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangesForForwardSrv);
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRangesForForwardSrv;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangesForForwardDsv);
	rootParameters[3].DescriptorTable.pDescriptorRanges = descriptorRangesForForwardDsv;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* signatureBlob;
	ID3DBlob* errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	hr = dxManager->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));
	return rootSignature;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> CompositePipeline::CreatePSO(
	DirectXManager* dxManager, ID3D12RootSignature* rootSignature)
{
	auto vertexShaderBlob = dxManager->CompileShader(L"./resource/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
	auto pixelShaderBlob = dxManager->CompileShader(L"./resource/shaders/Composite.PS.hlsl", L"ps_6_0");
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
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = dxManager->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	assert(SUCCEEDED(hr));
	return pso;
}

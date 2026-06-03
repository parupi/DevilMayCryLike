#include "OffScreenPipeline.h"
#include "Graphics/Device/DirectXManager.h"
#include "Utility/Logger.h"
#include <cassert>
#include <DirectXTex/d3dx12.h>

Microsoft::WRL::ComPtr<ID3D12RootSignature> OffScreenPipeline::CreateRootSignature(DirectXManager* dxManager)
{
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[2] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].Descriptor.ShaderRegister = 0;
	rootParameters[1].Descriptor.RegisterSpace = 0;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	hr = dxManager->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));
	return rootSignature;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> OffScreenPipeline::CreatePSO(
	DirectXManager* dxManager, ID3D12RootSignature* rootSignature, OffScreenEffectType effectType)
{
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = nullptr;
	inputLayoutDesc.NumElements = 0;

	IDxcBlob* vertexShaderBlob{};
	IDxcBlob* pixelShaderBlob{};

	switch (effectType) {
	case OffScreenEffectType::kNone:
		vertexShaderBlob = dxManager->CompileShader(L"./resource/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
		pixelShaderBlob = dxManager->CompileShader(L"./resource/shaders/CopyImage.PS.hlsl", L"ps_6_0");
		break;
	case OffScreenEffectType::kGray:
		vertexShaderBlob = dxManager->CompileShader(L"./resource/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
		pixelShaderBlob = dxManager->CompileShader(L"./resource/shaders/Grayscale.PS.hlsl", L"ps_6_0");
		break;
	case OffScreenEffectType::kVignette:
		vertexShaderBlob = dxManager->CompileShader(L"./resource/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
		pixelShaderBlob = dxManager->CompileShader(L"./resource/shaders/Vignette.PS.hlsl", L"ps_6_0");
		break;
	case OffScreenEffectType::kSmooth:
		vertexShaderBlob = dxManager->CompileShader(L"./resource/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
		pixelShaderBlob = dxManager->CompileShader(L"./resource/shaders/BoxFilter.PS.hlsl", L"ps_6_0");
		break;
	case OffScreenEffectType::kGauss:
		vertexShaderBlob = dxManager->CompileShader(L"./resource/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
		pixelShaderBlob = dxManager->CompileShader(L"./resource/shaders/GaussianFilter.PS.hlsl", L"ps_6_0");
		break;
	case OffScreenEffectType::kOutLine:
		vertexShaderBlob = dxManager->CompileShader(L"./resource/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
		pixelShaderBlob = dxManager->CompileShader(L"./resource/shaders/OutLine.PS.hlsl", L"ps_6_0");
		break;
	}
	assert(pixelShaderBlob != nullptr);

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = false;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature;
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = dxManager->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	assert(SUCCEEDED(hr));
	return pso;
}

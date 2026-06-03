#include "SkinningPipeline.h"
#include "Graphics/Device/DirectXManager.h"
#include "Utility/Logger.h"
#include <cassert>

Microsoft::WRL::ComPtr<ID3D12RootSignature> SkinningPipeline::CreateRootSignature(DirectXManager* dxManager)
{
	D3D12_DESCRIPTOR_RANGE srvRange0[1] = {};
	srvRange0[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange0[0].NumDescriptors = 1;
	srvRange0[0].BaseShaderRegister = 0; // t0
	srvRange0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE srvRange1[1] = {};
	srvRange1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange1[0].NumDescriptors = 1;
	srvRange1[0].BaseShaderRegister = 1; // t1
	srvRange1[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE srvRange2[1] = {};
	srvRange2[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange2[0].NumDescriptors = 1;
	srvRange2[0].BaseShaderRegister = 2; // t2
	srvRange2[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE uavRange[1] = {};
	uavRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	uavRange[0].NumDescriptors = 1;
	uavRange[0].BaseShaderRegister = 0; // u0
	uavRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[5] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(srvRange0);
	rootParameters[0].DescriptorTable.pDescriptorRanges = srvRange0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(srvRange1);
	rootParameters[1].DescriptorTable.pDescriptorRanges = srvRange1;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(srvRange2);
	rootParameters[2].DescriptorTable.pDescriptorRanges = srvRange2;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(uavRange);
	rootParameters[3].DescriptorTable.pDescriptorRanges = uavRange;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[4].Descriptor.ShaderRegister = 0;
	rootParameters[4].Descriptor.RegisterSpace = 0;
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature = {};
	descriptionRootSignature.NumParameters = _countof(rootParameters);
	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

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

Microsoft::WRL::ComPtr<ID3D12PipelineState> SkinningPipeline::CreatePSO(
	DirectXManager* dxManager, ID3D12RootSignature* rootSignature)
{
	Microsoft::WRL::ComPtr<IDxcBlob> computeShaderBlob = dxManager->CompileShader(L"./resource/shaders/Skinning.CS.hlsl", L"cs_6_0");
	assert(computeShaderBlob != nullptr);

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc{};
	computePsoDesc.CS = {
		.pShaderBytecode = computeShaderBlob->GetBufferPointer(),
		.BytecodeLength = computeShaderBlob->GetBufferSize()
	};
	computePsoDesc.pRootSignature = rootSignature;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = dxManager->GetDevice()->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&pso));
	assert(SUCCEEDED(hr));
	return pso;
}

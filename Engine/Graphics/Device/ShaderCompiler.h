#pragma once
#include <wrl.h>
#include <dxcapi.h>
#include <string>

class ShaderCompiler {
public:
	void Initialize();
	IDxcBlob* Compile(const std::wstring& filePath, const wchar_t* profile);

private:
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
};

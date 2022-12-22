#pragma once

namespace CM_HlslShaderToy
{
	class ShaderCompiler
	{
	public:
		ShaderCompiler();
		bool CompilePixelShader(LPCWSTR filepath, IDxcBlob** ppShaderOut);

	private:
		bool CompileShader(size_t numArgs, LPCWSTR* pszArgs, IDxcBlob** ppShaderOut);

	private:
		ComPtr<IDxcUtils> pUtils;
		ComPtr<IDxcCompiler3> pCompiler;
		ComPtr<IDxcIncludeHandler> pIncludeHandler;
	};
}
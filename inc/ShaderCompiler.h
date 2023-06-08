#pragma once

namespace CM_HlslShaderToy
{
	class ShaderCompiler
	{
	public:
		ShaderCompiler();
		bool CompilePixelShaderFromFile(LPCWSTR filepath, IDxcBlob** ppShaderOut);
		bool CompilePixelShaderFromText(size_t numChars, LPCWSTR pText, LPCWSTR filename, IDxcBlob** ppShaderOut);

	private:
		bool CompileShader(const DxcBuffer& sourceBuffer, size_t numArgs, LPCWSTR* pszArgs, IDxcBlob** ppShaderOut, IDxcBlobUtf8** ppErrors);

	private:
		ComPtr<IDxcUtils> pUtils;
		ComPtr<IDxcCompiler3> pCompiler;
		ComPtr<IDxcIncludeHandler> pIncludeHandler;
	};
}
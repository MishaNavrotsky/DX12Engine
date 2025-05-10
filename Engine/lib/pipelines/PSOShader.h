#pragma once

#include "../DXSampleHelper.h"
#include <filesystem>

namespace Engine {
	struct PSOShaderCreate {
		std::filesystem::path VS;
		std::filesystem::path PS;
		std::wstring VSEntry;
		std::wstring PSEntry;

	};
	using namespace Microsoft::WRL;
	class PSOShader {
	public:
		static std::unique_ptr<PSOShader> Create(const PSOShaderCreate& settings);
		IDxcBlob* getVS() const;
		IDxcBlob* getPS() const;
	private:
		static ComPtr<IDxcCompiler3> m_compiler;
		static ComPtr<IDxcLibrary> m_library;
		PSOShader(ComPtr<IDxcBlob> vs, ComPtr<IDxcBlob> ps);

		ComPtr<IDxcBlob> m_vs;
		ComPtr<IDxcBlob> m_ps;
	};
}

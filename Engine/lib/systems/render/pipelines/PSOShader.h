#pragma once

#include <filesystem>
#include "../../../helpers.h"

namespace Engine::Render::Pipeline {
	struct PSOShaderCreate {
		std::filesystem::path VS;
		std::filesystem::path PS;
		std::filesystem::path CS;

		std::wstring VSEntry;
		std::wstring PSEntry;
		std::wstring CSEntry;

	};

	class PSOShader {
	public:
		static std::unique_ptr<PSOShader> Create(const PSOShaderCreate& settings);
		IDxcBlob* getVS() const;
		IDxcBlob* getPS() const;
		IDxcBlob* getCOM() const;
	private:
		static WPtr<IDxcCompiler3> m_compiler;
		static WPtr<IDxcLibrary> m_library;
		PSOShader(WPtr<IDxcBlob> vs = nullptr, WPtr<IDxcBlob> ps = nullptr, WPtr<IDxcBlob> com = nullptr);

		WPtr<IDxcBlob> m_vs;
		WPtr<IDxcBlob> m_ps;
		WPtr<IDxcBlob> m_com;
	};
}
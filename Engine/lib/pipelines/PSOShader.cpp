#include "stdafx.h"
#include "PSOShader.h"

Microsoft::WRL::ComPtr<IDxcCompiler3> Engine::PSOShader::m_compiler = nullptr;
Microsoft::WRL::ComPtr<IDxcLibrary> Engine::PSOShader::m_library = nullptr;

std::unique_ptr<Engine::PSOShader> Engine::PSOShader::Create(const PSOShaderCreate& settings) {
	if (m_compiler.Get() == nullptr || m_library.Get() == nullptr) {
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)));
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library)));
	}

	auto vs = CompileShaderFromFile(m_compiler.Get(), m_library.Get(), settings.VS.c_str(), settings.VSEntry.c_str(), L"vs_6_6");
	auto ps = CompileShaderFromFile(m_compiler.Get(), m_library.Get(), settings.PS.c_str(), settings.PSEntry.c_str(), L"ps_6_6");

	return std::unique_ptr<PSOShader>(new PSOShader(vs, ps));
}

IDxcBlob* Engine::PSOShader::getVS() const {
	return m_vs.Get();
}

IDxcBlob* Engine::PSOShader::getPS() const {
	return m_ps.Get();
}

Engine::PSOShader::PSOShader(ComPtr<IDxcBlob> vs, ComPtr<IDxcBlob> ps) {
	m_vs = vs;
	m_ps = ps;
}

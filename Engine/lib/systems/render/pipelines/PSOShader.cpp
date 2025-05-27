#include "stdafx.h"
#include "PSOShader.h"

WPtr<IDxcCompiler3> Engine::Render::Pipeline::PSOShader::m_compiler = nullptr;
WPtr<IDxcLibrary> Engine::Render::Pipeline::PSOShader::m_library = nullptr;

std::unique_ptr<Engine::Render::Pipeline::PSOShader> Engine::Render::Pipeline::PSOShader::Create(const PSOShaderCreate& settings) {
	if (m_compiler.Get() == nullptr || m_library.Get() == nullptr) {
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)));
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library)));
	}

	WPtr<IDxcBlob> vs, ps, cs;
	if (!settings.VS.empty()) vs = Helpers::CompileShaderFromFile(m_compiler.Get(), m_library.Get(), settings.VS.c_str(), settings.VSEntry.c_str(), L"vs_6_6");
	if (!settings.PS.empty()) ps = Helpers::CompileShaderFromFile(m_compiler.Get(), m_library.Get(), settings.PS.c_str(), settings.PSEntry.c_str(), L"ps_6_6");
	if (!settings.CS.empty()) cs = Helpers::CompileShaderFromFile(m_compiler.Get(), m_library.Get(), settings.CS.c_str(), settings.CSEntry.c_str(), L"cs_6_6");

	return std::unique_ptr<PSOShader>(new PSOShader(vs, ps, cs));
}

IDxcBlob* Engine::Render::Pipeline::PSOShader::getVS() const {
	return m_vs.Get();
}

IDxcBlob* Engine::Render::Pipeline::PSOShader::getPS() const {
	return m_ps.Get();
}

IDxcBlob* Engine::Render::Pipeline::PSOShader::getCOM() const
{
	return m_com.Get();
}

Engine::Render::Pipeline::PSOShader::PSOShader(WPtr<IDxcBlob> vs, WPtr<IDxcBlob> ps, WPtr<IDxcBlob> com) {
	m_vs = vs;
	m_ps = ps;
	m_com = com;
}
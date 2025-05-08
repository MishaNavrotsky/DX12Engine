#include "stdafx.h"
#include "Device.h"

Microsoft::WRL::ComPtr<ID3D12Device> Engine::Device::m_device; // Define the static member

void Engine::Device::SetDevice(Microsoft::WRL::ComPtr<ID3D12Device> device) {
	m_device = device;
}

Microsoft::WRL::ComPtr<ID3D12Device> Engine::Device::GetDevice() {
	return m_device;
}

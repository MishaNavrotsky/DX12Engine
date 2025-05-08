#pragma once

namespace Engine {
	class Device {
	public:
		static void SetDevice(Microsoft::WRL::ComPtr<ID3D12Device> device);

		static Microsoft::WRL::ComPtr<ID3D12Device> GetDevice();

	private:
		static Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	};
}

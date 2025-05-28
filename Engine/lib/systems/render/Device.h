#include "stdafx.h"

#pragma once

#include <d3dx12.h>
#include <wrl.h>

namespace Engine::Render {
	class Device {
	public:
		static  WPtr<ID3D12Device> Initialize(IDXGIFactory4* factory,bool useWarpDevice) {
			m_useWarpDevice = useWarpDevice;

			if (useWarpDevice)
			{
				WPtr<IDXGIAdapter> warpAdapter;
				ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

				ThrowIfFailed(D3D12CreateDevice(
					warpAdapter.Get(),
					D3D_FEATURE_LEVEL_12_2,
					IID_PPV_ARGS(&m_device)
				));
			}
			else
			{
				WPtr<IDXGIAdapter1> hardwareAdapter;
				GetHardwareAdapter(factory, &hardwareAdapter, true);

				ThrowIfFailed(D3D12CreateDevice(
					hardwareAdapter.Get(),
					D3D_FEATURE_LEVEL_12_2,
					IID_PPV_ARGS(&m_device)
				));
			}
#if defined(_DEBUG)
			m_device->SetStablePowerState(TRUE);
#endif
			return m_device;
		}

		static WPtr<ID3D12Device> GetDevice() {
			return m_device;
		}

	private:
		static inline bool m_useWarpDevice;
		static inline WPtr<ID3D12Device> m_device;
		static void GetHardwareAdapter(
			    IDXGIFactory1* pFactory,
			    IDXGIAdapter1** ppAdapter,
			    bool requestHighPerformanceAdapter)
			{
			    *ppAdapter = nullptr;
			
			    WPtr<IDXGIAdapter1> adapter;
			
				WPtr<IDXGIFactory7> factory7;
			    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory7))))
			    {
			        for (
			            UINT adapterIndex = 0;
			            SUCCEEDED(factory7->EnumAdapterByGpuPreference(
			                adapterIndex,
			                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
			                IID_PPV_ARGS(&adapter)));
			            ++adapterIndex)
			        {
			            DXGI_ADAPTER_DESC1 desc;
			            adapter->GetDesc1(&desc);
			
			            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			            {
			                // Don't select the Basic Render Driver adapter.
			                // If you want a software adapter, pass in "/warp" on the command line.
			                continue;
			            }
			
			            // Check to see whether the adapter supports Direct3D 12, but don't create the
			            // actual device yet.
			            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, _uuidof(ID3D12Device), nullptr)))
			            {
			                break;
			            }
			        }
			    }
			
			    if(adapter.Get() == nullptr)
			    {
			        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
			        {
			            DXGI_ADAPTER_DESC1 desc;
			            adapter->GetDesc1(&desc);
			
			            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			            {
			                // Don't select the Basic Render Driver adapter.
			                // If you want a software adapter, pass in "/warp" on the command line.
			                continue;
			            }
			
			            // Check to see whether the adapter supports Direct3D 12, but don't create the
			            // actual device yet.
			            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, _uuidof(ID3D12Device), nullptr)))
			            {
			                break;
			            }
			        }
			    }
			    
			    *ppAdapter = adapter.Detach();
			}
	};
}
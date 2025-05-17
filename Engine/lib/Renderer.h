#pragma once

#include "DXSample.h"
#include "./camera/Camera.h"
#include "nodes/ModelSceneNode.h"
#include "scene/Scene.h"
#include "queues/GPUUploadQueue.h"
#include "loaders/ModelLoader.h"
#include "descriptors/BindlessHeapDescriptor.h"
#include "Device.h"
#include "pipelines/PSOShader.h"
#include "pipelines/GBufferPass.h"
#include "pipelines/LightingPass.h"
#include "geometry/ModelMatrix.h"
#include "pipelines/gizmos/GizmosPass.h"
#include "mesh/InitializeDefault.h"
#include "pipelines/CompositionPass.h"


using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU. 
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class Renderer : public DXSample
{
    struct CommandLists {
        std::array<ID3D12CommandList*, 2> d_gbuffer;
        std::array<ID3D12CommandList*, 2> d_gizmos;
        std::array<ID3D12CommandList*, 1> c_lighting;
        std::array<ID3D12CommandList*, 2> d_composition;
    };
public:
    Renderer(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();
    virtual void OnKeyDown() override;
    virtual void OnMouseMove() override;

private:
    static const UINT FrameCount = 2;

    struct Vertex
    {
        XMFLOAT3 position;
    };

    // Pipeline objects.
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    UINT m_rtvDescriptorSize;

    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue = 0;

    std::unique_ptr <Engine::Camera> m_camera;
    Engine::Scene m_scene;
    Engine::GPUUploadQueue& m_uploadQueue = Engine::GPUUploadQueue::GetInstance();
    Engine::ModelLoader& m_modelLoader = Engine::ModelLoader::GetInstance();
    Engine::BindlessHeapDescriptor& m_bindlessHeapDescriptor = Engine::BindlessHeapDescriptor::GetInstance();



    std::unique_ptr<Engine::GBufferPass> m_gbufferPass;
    std::unique_ptr<Engine::LightingPass> m_lightingPass;
    std::unique_ptr<Engine::GizmosPass> m_gizmosPass;
    std::unique_ptr<Engine::CompositionPass> m_compositionPass;


    ComPtr<ID3D12CommandQueue> m_directCommandQueue;
    ComPtr<ID3D12CommandQueue> m_computeCommandQueue;


    float yaw = 0;
    float pitch = 0;

    void LoadPipeline();
    void LoadAssets();
    CommandLists PopulateCommandLists();
    void WaitForCommandQueueExecute();
};

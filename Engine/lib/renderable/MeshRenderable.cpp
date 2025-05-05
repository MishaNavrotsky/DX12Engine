#include "stdafx.h"
#include "MeshRenderable.h"

void Engine::MeshRenderable::render(ID3D12GraphicsCommandList* commandList)
{
    // Set vertex buffer views
    D3D12_VERTEX_BUFFER_VIEW vertexBuffers[] = {
        *m_gpuMesh.GetVertexBufferView(),
        * m_gpuMesh.GetNormalsBufferView(),
        * m_gpuMesh.GetTexCoordsBufferView(),
        * m_gpuMesh.GetTangentsBufferView()
    };

    commandList->IASetVertexBuffers(0, _countof(vertexBuffers), vertexBuffers);
    commandList->IASetIndexBuffer(m_gpuMesh.GetIndexBufferView());
    UINT indexCount = m_gpuMesh.GetIndexBufferView()->SizeInBytes / sizeof(UINT);
    commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

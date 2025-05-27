#include "stdafx.h"

#pragma once
#include "DirectXMath.h"
#include <iostream>
#include "../Device.h"

namespace Engine {
	struct CBVCameraData {
		XMFLOAT4X4 projectionMatrix;
		XMFLOAT4X4 projectionReverseDepthMatrix;
		XMFLOAT4X4 viewMatrix;
		XMFLOAT4X4 viewProjectionReverseDepthMatrix;

		XMFLOAT4X4 prevProjectionMatrix;
		XMFLOAT4X4 prevProjectionReverseDepthMatrix;
		XMFLOAT4X4 prevViewMatrix;
		XMFLOAT4X4 prevViewProjectionReverseDepthMatrix;
		XMFLOAT4 position;
		XMUINT4 dimensions;
	};

	class Camera {
	public:
		struct Frustum {
			// x,y,z = normal, w = distance
			DX::XMVECTOR planes[6]; // 0: left, 1: right, 2: top, 3: bottom, 4: near, 5: far
		};

		Camera(float fov, int width, int height, float nearPlane, float farPlane) :
			m_fov(fov), m_width(width), m_height(height), m_nearPlane(nearPlane), m_farPlane(farPlane) {
			m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

			updateProjectionMatrix();
			updateViewMatrix();

			m_prevViewMatrix = m_viewMatrix;
			m_prevReverseZProjectionMatrix = m_reverseZProjectionMatrix;
			m_prevProjectionMatrix = m_projectionMatrix;

			auto sizeInBytesWithAlignment = (static_cast<UINT>(sizeof(CBVCameraData)) + 255) & ~255;

			D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytesWithAlignment);
			bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

			ThrowIfFailed(m_device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_cbv)));
			m_cbv->SetName(L"Camera Buffer");

			update();
		}

		void setPosition(XMVECTOR position) {
			position.m128_f32[3] = 1.f;
			m_position = position;
		}
		void setLookAt(XMVECTOR lookAt) {
			lookAt.m128_f32[3] = 1.f;
			m_lookAt = lookAt;
		}

		void update() {
			updateViewMatrix();


			CBVCameraData cbvData;
			auto cameraProjection = getProjectionMatrix();
			auto cameraProjectionReverseDepth = getProjectionMatrixForReverseDepth();
			auto cameraView = getViewMatrix();
			m_cameraViewProjReverseDepth = cameraView * cameraProjectionReverseDepth;
			XMStoreFloat4x4(&cbvData.projectionMatrix, cameraProjection);
			XMStoreFloat4x4(&cbvData.projectionReverseDepthMatrix, cameraProjectionReverseDepth);
			XMStoreFloat4x4(&cbvData.viewMatrix, cameraView);
			auto t = XMMatrixTranspose(m_cameraViewProjReverseDepth);
			XMStoreFloat4x4(&cbvData.viewProjectionReverseDepthMatrix, t);
			ExtractFrustumPlanes(t);

			auto cameraPrevProjection = getPrevProjectionMatrix();
			auto cameraPrevProjectionReverseDepth = getPrevProjectionMatrixForReverseDepth();
			auto cameraPrevView = getPrevViewMatrix();
			XMStoreFloat4x4(&cbvData.prevProjectionMatrix, cameraPrevProjection);
			XMStoreFloat4x4(&cbvData.prevProjectionReverseDepthMatrix, cameraPrevProjectionReverseDepth);
			XMStoreFloat4x4(&cbvData.prevViewMatrix, cameraPrevView);
			XMStoreFloat4x4(&cbvData.prevViewProjectionReverseDepthMatrix, XMMatrixTranspose(cameraPrevView * cameraPrevProjectionReverseDepth));

			XMStoreFloat4(&cbvData.position, getPosition());
			XMVECTOR dimensions = XMVectorSet((float)m_width, (float)m_height, 0.0f, 0.0f);
			XMStoreUInt4(&cbvData.dimensions, dimensions);


			void* mappedData = nullptr;
			m_cbv->Map(0, nullptr, &mappedData);

			memcpy(mappedData, &cbvData, sizeof(CBVCameraData));

			m_cbv->Unmap(0, nullptr);
		}

		ID3D12Resource* getResource() const {
			return m_cbv.Get();
		}

		XMMATRIX getProjectionMatrix() const {
			return m_projectionMatrix;
		}
		XMMATRIX getPrevProjectionMatrix() const {
			return m_prevProjectionMatrix;
		}

		XMMATRIX getProjectionMatrixForReverseDepth() const {
			return m_reverseZProjectionMatrix;
		}
		XMMATRIX getPrevProjectionMatrixForReverseDepth() const {
			return m_prevReverseZProjectionMatrix;
		}
		XMMATRIX getViewMatrix() const {
			return m_viewMatrix;
		}
		XMMATRIX getPrevViewMatrix() const {
			return m_prevViewMatrix;
		}
		XMVECTOR getPosition() const {
			return m_position;
		}
		XMVECTOR getLookAt() const {
			return m_lookAt;
		}
		Frustum& getFrustum() {
			return m_frustum;
		}
		XMMATRIX getViewProjReverseDepth() const {
			return m_cameraViewProjReverseDepth;
		}
	private:
		XMMATRIX m_viewMatrix = XMMatrixIdentity();
		XMMATRIX m_projectionMatrix = XMMatrixIdentity();
		XMVECTOR m_position = XMVectorSet(0, 0, 0, 0);
		XMVECTOR m_lookAt = XMVectorSet(1, 0, 0, 0);
		XMMATRIX m_prevViewMatrix = XMMatrixIdentity();
		XMMATRIX m_prevProjectionMatrix = XMMatrixIdentity();
		XMMATRIX m_reverseZProjectionMatrix = XMMatrixIdentity();
		XMMATRIX m_cameraViewProjReverseDepth = XMMatrixIdentity();
		XMMATRIX m_prevReverseZProjectionMatrix = XMMatrixIdentity();
		Frustum m_frustum;

		ComPtr<ID3D12Resource> m_cbv;
		ComPtr<ID3D12Device> m_device = Device::GetDevice();


		float m_fov;
		int m_width;
		int m_height;
		float m_nearPlane;
		float m_farPlane;
		float m_aspectRatio;

		void updateViewMatrix() {
			m_prevViewMatrix = m_viewMatrix;
			m_viewMatrix = XMMatrixLookAtLH(
				m_position,
				m_position + m_lookAt,
				XMVectorSet(0.f, 1.f, 0.f, 0.f)
			);
		}

		void updateProjectionMatrix() {
			m_prevProjectionMatrix = m_projectionMatrix;
			m_projectionMatrix = XMMatrixPerspectiveFovLH(
				m_fov,
				m_aspectRatio,
				m_nearPlane,
				m_farPlane
			);

			m_prevReverseZProjectionMatrix = m_reverseZProjectionMatrix;
			m_reverseZProjectionMatrix = XMMatrixPerspectiveFovLH(
				m_fov,
				m_aspectRatio,
				m_farPlane,
				m_nearPlane
			);
		}

		void ExtractFrustumPlanes(const XMMATRIX& m)
		{
			m_frustum.planes[0] = m.r[3] + m.r[0]; // left
			m_frustum.planes[1] = m.r[3] - m.r[0]; // right 
			m_frustum.planes[2] = m.r[3] + m.r[1]; // bottom
			m_frustum.planes[3] = m.r[3] - m.r[1]; // top
			m_frustum.planes[4] = m.r[3] - m.r[2]; // far
			m_frustum.planes[5] = m.r[2]; // near
			//m_frustum.planes[5] = m.r[3] + m.r[2]; // near
		}
	};
}
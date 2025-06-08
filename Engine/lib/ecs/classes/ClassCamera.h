#include "stdafx.h"

#pragma once

#include "../components/ComponentCamera.h"
#include "../components/ComponentTransform.h"


namespace Engine::ECS::Class {
	class ClassCamera {
	public:
		struct CameraData {
			DX::XMFLOAT4X4 viewMatrix;
			DX::XMFLOAT4X4 viewReverseProjMatrix;

			DX::XMFLOAT4 position;
		};

		ClassCamera(Component::ComponentCamera& componentCamera, Component::ComponentTransform& componentTransform) {
			update(componentCamera, componentTransform);
		}

		inline void update(Component::ComponentCamera& componentCamera, Component::ComponentTransform& componentTransform) {
			setComponentCamera(componentCamera);
			setComponentTransform(componentTransform);
			updateProjectionMatrix();
			updateViewMatrix();
		}

		inline void setComponentCamera(Component::ComponentCamera& componentCamera) {
			m_fov = componentCamera.fov;
			m_nearPlane = componentCamera.nearPlane;
			m_farPlane = componentCamera.farPlane;
			m_aspectRatio = componentCamera.aspectRatio;
		}

		inline void setComponentTransform(Component::ComponentTransform& componentTransform) {
			m_position = DX::XMLoadFloat4(&componentTransform.position);
			m_rotationQat = DX::XMLoadFloat4(&componentTransform.rotation);
		}

		std::unique_ptr<CameraData> getCameraData() {
			std::unique_ptr<CameraData> cameraData = std::make_unique<CameraData>();
			DX::XMStoreFloat4(&cameraData->position, m_position);
			DX::XMStoreFloat4x4(&cameraData->viewMatrix, DX::XMMatrixTranspose(m_viewMatrix));
			DX::XMStoreFloat4x4(&cameraData->viewReverseProjMatrix, DX::XMMatrixTranspose(m_viewMatrix * m_reverseProjectionMatrix));


			return cameraData;
		}

		std::array<DX::XMVECTOR, 6> extractFrustumPlanes(const DX::XMMATRIX& m)
		{
			std::array<DX::XMVECTOR, 6> m_frustumPlanes;
			m_frustumPlanes[0] = DX::XMVectorAdd(m.r[3], m.r[0]); // left
			m_frustumPlanes[1] = DX::XMVectorSubtract(m.r[3], m.r[0]); // right 
			m_frustumPlanes[2] = DX::XMVectorAdd(m.r[3], m.r[1]); // bottom
			m_frustumPlanes[3] = DX::XMVectorSubtract(m.r[3], m.r[1]); // top
			m_frustumPlanes[4] = DX::XMVectorSubtract(m.r[3], m.r[2]); // far
			m_frustumPlanes[5] = m.r[2]; // near
			//m_frustum.planes[5] = m.r[3] + m.r[2]; // near
			return m_frustumPlanes;
		}
	private:
		DX::XMMATRIX m_viewMatrix = DX::XMMatrixIdentity();
		DX::XMMATRIX m_reverseProjectionMatrix = DX::XMMatrixIdentity();
		DX::XMMATRIX m_viewReverseProjMatrix;

		DX::XMVECTOR m_position = DX::XMVectorSet(0, 0, 0, 1);
		DX::XMVECTOR m_rotationQat = DX::XMVectorSet(1, 0, 0, 0);
		DX::XMVECTOR m_frustumPlanes[6];

		float m_fov, m_nearPlane, m_farPlane, m_aspectRatio;

		void updateViewMatrix() {
			DX::XMMATRIX rotMat = DX::XMMatrixRotationQuaternion(m_rotationQat);
			DX::XMMATRIX transMat = DX::XMMatrixTranslationFromVector(m_position);
			DX::XMMATRIX worldMatrix = rotMat * transMat;
			m_viewMatrix = DX::XMMatrixInverse(nullptr, worldMatrix);
		}

		void updateProjectionMatrix() {
			m_reverseProjectionMatrix = DX::XMMatrixPerspectiveFovLH(
				m_fov,
				m_aspectRatio,
				m_farPlane,
				m_nearPlane
			);
		}

	};
}

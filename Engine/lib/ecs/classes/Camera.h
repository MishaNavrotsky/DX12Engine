#include "stdafx.h"

#pragma once

#include "../components/ComponentCamera.h"
#include "../components/ComponentTransform.h"


namespace Engine::ECS::Class {
	class Camera {
	public:
		struct CameraData {
			DX::XMFLOAT4X4 viewMatrix;
			DX::XMFLOAT4X4 prevViewMatrix;

			DX::XMFLOAT4X4 projectionMatrix;
			DX::XMFLOAT4X4 prevProjectionMatrix;

			DX::XMFLOAT4X4 reverseProjectionMatrix;
			DX::XMFLOAT4X4 prevReverseProjectionMatrix;

			DX::XMFLOAT4X4 viewReverseProjMatrix;
			DX::XMFLOAT4X4 prevViewReverseProjMatrix;

			DX::XMFLOAT4 position;
			DX::XMFLOAT4 lookAt;
		};

		Camera(Component::ComponentCamera componentCamera, Component::ComponentTransform componentTransform) : m_componentCamera(componentCamera), m_componentTransform(componentTransform) {

			updateProjectionMatrix();
			updateViewMatrix();

			m_prevViewMatrix = m_viewMatrix;
			m_prevReverseProjectionMatrix = m_reverseProjectionMatrix;
			m_prevProjectionMatrix = m_projectionMatrix;

			update();
		}

		void setPosition(DX::XMVECTOR position) {
			position.m128_f32[3] = 1.f;
			m_position = position;
		}
		void setLookAt(DX::XMVECTOR lookAt) {
			lookAt.m128_f32[3] = 1.f;
			m_lookAt = lookAt;
		}

		void update() {
			updateViewMatrix();
		}

		Component::ComponentCamera& getComponent() {
			return m_componentCamera;
		}
	private:
		DX::XMMATRIX m_viewMatrix = DX::XMMatrixIdentity();
		DX::XMMATRIX m_prevViewMatrix = DX::XMMatrixIdentity();

		DX::XMMATRIX m_projectionMatrix = DX::XMMatrixIdentity();
		DX::XMMATRIX m_prevProjectionMatrix = DX::XMMatrixIdentity();

		DX::XMMATRIX m_reverseProjectionMatrix = DX::XMMatrixIdentity();
		DX::XMMATRIX m_prevReverseProjectionMatrix = DX::XMMatrixIdentity();

		DX::XMMATRIX m_viewReverseProjMatrix = DX::XMMatrixIdentity();
		DX::XMMATRIX m_prevViewReverseProjMatrix = DX::XMMatrixIdentity();


		DX::XMVECTOR m_position = DX::XMVectorSet(0, 0, 0, 0);
		DX::XMVECTOR m_lookAt = DX::XMVectorSet(1, 0, 0, 0);
		DX::XMVECTOR m_frustumPlanes[6];

		Component::ComponentCamera m_componentCamera;
		Component::ComponentTransform m_componentTransform;


		float m_fov, m_nearPlane, m_farPlane, m_aspectRatio;
		int m_width, m_height;

		void updateViewMatrix() {
			m_prevViewMatrix = m_viewMatrix;
			m_viewMatrix = DX::XMMatrixLookAtLH(
				m_position,
				DX::XMVectorAdd(m_position, m_lookAt),
				DX::XMVectorSet(0.f, 1.f, 0.f, 0.f)
			);
		}

		void updateProjectionMatrix() {
			m_prevProjectionMatrix = m_projectionMatrix;
			m_projectionMatrix = DX::XMMatrixPerspectiveFovLH(
				m_fov,
				m_aspectRatio,
				m_nearPlane,
				m_farPlane
			);

			m_prevReverseProjectionMatrix = m_reverseProjectionMatrix;
			m_reverseProjectionMatrix = DX::XMMatrixPerspectiveFovLH(
				m_fov,
				m_aspectRatio,
				m_farPlane,
				m_nearPlane
			);
		}

		void ExtractFrustumPlanes(const DX::XMMATRIX& m)
		{
			m_frustumPlanes[0] = DX::XMVectorAdd(m.r[3], m.r[0]); // left
			m_frustumPlanes[1] = DX::XMVectorSubtract(m.r[3], m.r[0]); // right 
			m_frustumPlanes[2] = DX::XMVectorAdd(m.r[3], m.r[1]); // bottom
			m_frustumPlanes[3] = DX::XMVectorSubtract(m.r[3], m.r[1]); // top
			m_frustumPlanes[4] = DX::XMVectorSubtract(m.r[3], m.r[2]); // far
			m_frustumPlanes[5] = m.r[2]; // near
			//m_frustum.planes[5] = m.r[3] + m.r[2]; // near
		}
	};
}

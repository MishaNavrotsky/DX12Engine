#include "stdafx.h"

#pragma once
#include "DirectXMath.h"
#include <iostream>

namespace Engine {
	using namespace DirectX;
	class Camera {
	public:
		Camera(float fov, int width, int height, float nearPlane, float farPlane) :
			m_fov(fov), m_width(width), m_height(height), m_nearPlane(nearPlane), m_farPlane(farPlane) {
			m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

			updateProjectionMatrix();
			updateViewMatrix();
		}

		void setPosition(XMVECTOR position) {
			position.m128_f32[3] = 1.f;
			m_position = position;
			updateViewMatrix();
		}
		void setLookAt(XMVECTOR lookAt) {
			lookAt.m128_f32[3] = 1.f;
			m_lookAt = lookAt;
			updateViewMatrix();
		}

		XMMATRIX getProjectionMatrix() const {
			return m_projectionMatrix;
		}
		XMMATRIX getProjectionMatrixForReverseDepth() const {
			return XMMatrixPerspectiveFovLH(
				m_fov,
				m_aspectRatio,
				m_farPlane,
				m_nearPlane
			);
		}
		XMMATRIX getViewMatrix() const {
			return m_viewMatrix;
		}
		XMVECTOR getPosition() const {
			return m_position;
		}
		XMVECTOR getLookAt() const {
			return m_lookAt;
		}
	private:
		XMMATRIX m_viewMatrix = XMMatrixIdentity();
		XMMATRIX m_projectionMatrix = XMMatrixIdentity();
		XMVECTOR m_position = XMVectorSet(0, 0, -5, 1);
		XMVECTOR m_lookAt = XMVectorSet(1, 0, 0, 1);


		float m_fov;
		int m_width;
		int m_height;
		float m_nearPlane;
		float m_farPlane;
		float m_aspectRatio;

		void updateViewMatrix() {
			m_viewMatrix = XMMatrixLookAtLH(
				m_position,
				m_position + m_lookAt,
				XMVectorSet(0.f, 1.f, 0.f, 0.f)
			);
		}

		void updateProjectionMatrix() {
			m_projectionMatrix = XMMatrixPerspectiveFovLH(
				m_fov,
				m_aspectRatio,
				m_nearPlane,
				m_farPlane
			);
		}
	};
}
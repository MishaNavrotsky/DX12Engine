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

			m_prevViewMatrix = m_viewMatrix;
			m_prevReverseZProjectionMatrix = m_reverseZProjectionMatrix;
			m_prevProjectionMatrix = m_projectionMatrix;
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
	private:
		XMMATRIX m_viewMatrix = XMMatrixIdentity();
		XMMATRIX m_projectionMatrix = XMMatrixIdentity();
		XMVECTOR m_position = XMVectorSet(0, 0, -5, 1);
		XMVECTOR m_lookAt = XMVectorSet(1, 0, 0, 1);
		XMMATRIX m_prevViewMatrix = XMMatrixIdentity();
		XMMATRIX m_prevProjectionMatrix = XMMatrixIdentity();
		XMMATRIX m_reverseZProjectionMatrix = XMMatrixIdentity();
		XMMATRIX m_prevReverseZProjectionMatrix = XMMatrixIdentity();


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
	};
}
#include "stdafx.h"
#pragma once


namespace Engine {
	using namespace DirectX;
	class ModelMatrix {
	public:
		ModelMatrix() {
			updateMatrix();
		}
		void setPosition(float x, float y, float z) {
			m_position = XMVectorSet(x, y, z, 0.0f);
			updateMatrix();
		}
		void setRotation(float pitch, float yaw, float roll) {
			XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
			m_quaternion = XMQuaternionRotationMatrix(rotationMatrix);
			updateMatrix();
		}
		void setQuaternion(float x, float y, float z, float w) {
			m_quaternion = XMVectorSet(x, y, z, w);
			updateMatrix();
		}
		void setScale(float x, float y, float z) {
			m_scale = XMVectorSet(x, y, z, 0.0f);
			updateMatrix();
		}

		const XMMATRIX& getModelMatrix() const {
			return m_model;
		}
	private:
		void updateMatrix() {
			XMMATRIX translationMatrix = XMMatrixTranslationFromVector(m_position);
			XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(m_quaternion);
			XMMATRIX scalingMatrix = XMMatrixScalingFromVector(m_scale);
			m_model = XMMatrixMultiply(scalingMatrix, XMMatrixMultiply(rotationMatrix, translationMatrix));
		}
		XMMATRIX m_model;
		XMVECTOR m_position = XMVectorSet(0.0, 0.0, 0.0, 0.0);
		XMVECTOR m_quaternion = XMQuaternionRotationRollPitchYaw(0., 0., 0.);
		XMVECTOR m_scale = XMVectorSet(1.0, 1.0, 1.0, 1.0);
	};
}

#include "stdafx.h"
#pragma once


namespace Engine {
	using namespace DirectX;
	class ModelMatrix {
	public:
		ModelMatrix() {
			updateMatrix();
			m_prevModel = m_model;
		}
		void setPosition(float x, float y, float z) {
			m_position = XMVectorSet(x, y, z, 0.0f);
			isDirty = true;
		}
		void setRotation(float pitch, float yaw, float roll) {
			XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
			m_quaternion = XMQuaternionRotationMatrix(rotationMatrix);
			isDirty = true;
		}
		void setQuaternion(float x, float y, float z, float w) {
			m_quaternion = XMVectorSet(x, y, z, w);
			isDirty = true;
		}
		void setScale(float x, float y, float z) {
			m_scale = XMVectorSet(x, y, z, 0.0f);
			isDirty = true;
		}

		const XMMATRIX& getModelMatrix() const {
			return m_model;
		}

		const XMMATRIX& getPrevModelMatrix() const {
			return m_prevModel;
		}

		void update() {
			if (!isDirty) return;
			updateMatrix();
		}
	private:
		bool isDirty = false;
		void updateMatrix() {
			m_prevModel = m_model;
			XMMATRIX translationMatrix = XMMatrixTranslationFromVector(m_position);
			XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(m_quaternion);
			XMMATRIX scalingMatrix = XMMatrixScalingFromVector(m_scale);
			m_model = XMMatrixMultiply(scalingMatrix, XMMatrixMultiply(rotationMatrix, translationMatrix));
			isDirty = false;
		}
		XMMATRIX m_model;
		XMMATRIX m_prevModel;
		XMVECTOR m_position = XMVectorSet(0.0, 0.0, 0.0, 0.0);
		XMVECTOR m_quaternion = XMQuaternionRotationRollPitchYaw(0., 0., 0.);
		XMVECTOR m_scale = XMVectorSet(1.0, 1.0, 1.0, 1.0);
	};
}

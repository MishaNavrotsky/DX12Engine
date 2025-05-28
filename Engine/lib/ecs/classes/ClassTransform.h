#include "stdafx.h"
#pragma once



#include "../components/ComponentTransform.h"


namespace Engine::ECS::Class {
	class ClassTransform {
	public:
		ClassTransform(Component::ComponentTransform componentTransform): m_componentTransform(componentTransform) {
			updateMatrix();
			m_prevModel = m_model;
		}
		void setPosition(float x, float y, float z) {
			m_position = DX::XMVectorSet(x, y, z, 0.0f);
			isDirty = true;
		}
		void setRotation(float pitch, float yaw, float roll) {
			DX::XMMATRIX rotationMatrix = DX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
			m_quaternion = DX::XMQuaternionRotationMatrix(rotationMatrix);
			isDirty = true;
		}
		void setQuaternion(float x, float y, float z, float w) {
			m_quaternion = DX::XMVectorSet(x, y, z, w);
			isDirty = true;
		}
		void setScale(float x, float y, float z) {
			m_scale = DX::XMVectorSet(x, y, z, 0.0f);
			isDirty = true;
		}

		const DX::XMMATRIX& getModelMatrix() const {
			return m_model;
		}

		const DX::XMMATRIX& getPrevModelMatrix() const {
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
			DX::XMMATRIX translationMatrix = DX::XMMatrixTranslationFromVector(m_position);
			DX::XMMATRIX rotationMatrix = DX::XMMatrixRotationQuaternion(m_quaternion);
			DX::XMMATRIX scalingMatrix = DX::XMMatrixScalingFromVector(m_scale);
			m_model = DX::XMMatrixMultiply(scalingMatrix, XMMatrixMultiply(rotationMatrix, translationMatrix));
			isDirty = false;
		}
		DX::XMMATRIX m_model;
		DX::XMMATRIX m_prevModel;
		DX::XMVECTOR m_position = DX::XMVectorSet(0.0, 0.0, 0.0, 0.0);
		DX::XMVECTOR m_quaternion = DX::XMQuaternionRotationRollPitchYaw(0., 0., 0.);
		DX::XMVECTOR m_scale = DX::XMVectorSet(1.0, 1.0, 1.0, 1.0);

		Component::ComponentTransform m_componentTransform;
	};
}

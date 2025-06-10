#include "stdafx.h"
#pragma once



#include "../components/ComponentTransform.h"


namespace Engine::ECS::Class {
	class ClassTransform {
	public:
		struct TransformData { 
			DX::XMFLOAT4X4 modelMatrix; 
			DX::XMFLOAT4 position;
		};
		ClassTransform(const Component::ComponentTransform& componentTransform) {
			update(componentTransform);
		}
		void setPosition(float x, float y, float z) {
			m_position = DX::XMVectorSet(x, y, z, 0.0f);
		}
		void setRotation(float pitch, float yaw, float roll) {
			DX::XMMATRIX rotationMatrix = DX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
			m_rotationQat = DX::XMQuaternionRotationMatrix(rotationMatrix);
		}
		void setQuaternion(float x, float y, float z, float w) {
			m_rotationQat = DX::XMVectorSet(x, y, z, w);
		}
		void setScale(float x, float y, float z) {
			m_scale = DX::XMVectorSet(x, y, z, 0.0f);
		}

		std::unique_ptr<TransformData> getTransformData() const {
			std::unique_ptr<TransformData> td = std::make_unique<TransformData>();
			DX::XMStoreFloat4x4(&td->modelMatrix, DX::XMMatrixTranspose(m_model));
			DX::XMStoreFloat4(&td->position, m_position);
			return td;
		}

		void update(const Component::ComponentTransform& componentTransform) {
			m_position = DX::XMLoadFloat4(&componentTransform.position);
			m_rotationQat = DX::XMLoadFloat4(&componentTransform.rotation);
			m_scale = DX::XMLoadFloat4(&componentTransform.scale);

			updateMatrix();
		}
	private:
		void updateMatrix() {
			DX::XMMATRIX translationMatrix = DX::XMMatrixTranslationFromVector(m_position);
			DX::XMMATRIX rotationMatrix = DX::XMMatrixRotationQuaternion(m_rotationQat);
			DX::XMMATRIX scalingMatrix = DX::XMMatrixScalingFromVector(m_scale);
			m_model = DX::XMMatrixMultiply(scalingMatrix, XMMatrixMultiply(rotationMatrix, translationMatrix));
			//m_model = DX::XMMatrixMultiply(translationMatrix, XMMatrixMultiply(rotationMatrix, scalingMatrix));
		}
		DX::XMMATRIX m_model;
		DX::XMVECTOR m_position;
		DX::XMVECTOR m_rotationQat;
		DX::XMVECTOR m_scale;
	};
}

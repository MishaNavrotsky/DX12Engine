#include "stdafx.h"

#pragma once

#include <DirectXMath.h>
#include "../geometry/ModelMatrix.h"
#include "../camera/Camera.h"
#include "../mesh/CPUMesh.h"
#include "../mesh/GPUMesh.h"

namespace Engine {
	enum class SceneNodeType {
		Undefined = 0,
		Mesh,              // A renderable mesh
		Light,             // Light (point, spot, directional)
		Camera,            // Camera node
	};
	class SceneNode {
	public:
		SceneNode() = default;

		void setLocalModelMatrix(const Engine::ModelMatrix& modelMatrix) {
			m_localMatrix = modelMatrix;
			markUpdate();
		}

		const XMMATRIX& getLocalModelMatrix() {
			return m_localMatrix.getModelMatrix();
		}

		void addChild(std::shared_ptr<SceneNode> child) {
			m_children.push_back(child);
		}

		virtual void update(const XMMATRIX& parentWorldMatrix = XMMatrixIdentity()) {
			if (shouldUpdate) {
				m_worldMatrix = XMMatrixMultiply(m_localMatrix.getModelMatrix(), parentWorldMatrix);
				m_prevWorldMatrix = XMMatrixMultiply(m_localMatrix.getPrevModelMatrix(), parentWorldMatrix);
				shouldUpdate = false;
			}


			for (auto& child : m_children) {
				child->update(m_worldMatrix);
			}
		}

		const XMMATRIX& getWorldMatrix() const { return m_worldMatrix; }
		const XMMATRIX& getPrevWorldMatrix() const { return m_prevWorldMatrix; }

		virtual void draw(ID3D12GraphicsCommandList* commandList, Camera* camera, const std::function<bool(CPUMesh&, GPUMesh&, SceneNode* node)>& callback) {
			for (auto& child : m_children) {
				child->draw(commandList, camera, callback);
			}
		}

		virtual ID3D12Resource* getResource() {
			return nullptr;
		}

		bool getShouldUpdate() const {
			return shouldUpdate;
		}

		std::vector<std::shared_ptr<SceneNode>>& getChildren() {
			return m_children;
		}

		virtual SceneNodeType getType() const {
			return SceneNodeType::Undefined;
		}

		virtual AABB& getWorldSpaceAABB() {
			throw std::runtime_error("Shouldn't be called for not Mesh type");
		}

		virtual ~SceneNode() = default;
	protected:
		void markUpdate() {
			if (!shouldUpdate) {
				shouldUpdate = true;
				for (auto& child : m_children) {
					child->markUpdate();
				}
			}
		}
		ModelMatrix m_localMatrix;
		XMMATRIX m_worldMatrix;
		XMMATRIX m_prevWorldMatrix;
		bool shouldUpdate = true;

		std::vector<std::shared_ptr<SceneNode>> m_children;
	};
}
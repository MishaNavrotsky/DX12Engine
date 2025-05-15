#include "stdafx.h"
#pragma once
#include "SceneNode.h"

#include "../mesh/CPUMesh.h"
#include "../mesh/CPUMaterial.h"


#include "../Device.h"


namespace Engine {
	class Scene {
	public:
		uint64_t addNode(std::shared_ptr<SceneNode> sceneNode) noexcept {
			m_scene.push_back(sceneNode);
			return m_idIncreemntal++;
		}

		void draw(ID3D12GraphicsCommandList* commandList, Camera* camera, bool enableFrustumCulling, const std::function<bool(CPUMesh&, CPUMaterial&, SceneNode* node)>& callback) {
			handleSceneNodesShouldUpdate();
			for (auto& sceneNode : m_scene) {
				sceneNode->draw(commandList, camera, enableFrustumCulling, callback);
			}
		}

		std::vector<std::shared_ptr<SceneNode>>& getSceneRootNodes() {
			return m_scene;
		}

		std::vector<std::shared_ptr<SceneNode>> getAllMeshNodes() {
			std::vector<std::shared_ptr<SceneNode>> sceneMeshNodes;
			for (auto& node : m_scene) {
				getMeshNodesRec(node, sceneMeshNodes);
			}
			return sceneMeshNodes;
		}
	private:
		void getMeshNodesRec(std::shared_ptr<SceneNode> node, std::vector<std::shared_ptr<SceneNode>>& sceneMeshNodes) {
			auto& children = node->getChildren();
			for (auto& child : children) {
				if (child->getType() == SceneNodeType::Mesh) {
					sceneMeshNodes.push_back(child);
				}
				getMeshNodesRec(child, sceneMeshNodes);
			}
		}

		void updateNodeBranch(std::shared_ptr<SceneNode>& node, const XMMATRIX& parentWorldMatrix = XMMatrixIdentity()) {
			if (node->getShouldUpdate()) {
				node->update(parentWorldMatrix);
				return;
			}

			const XMMATRIX& currentWorldMatrix = XMMatrixMultiply(node->getLocalModelMatrix(), parentWorldMatrix);

			for (auto& child : node->getChildren()) {
				updateNodeBranch(child, currentWorldMatrix);
			}
		}

		void handleSceneNodesShouldUpdate() {
			for (auto& node : m_scene) {
				updateNodeBranch(node);
			}
		}
		std::vector<std::shared_ptr<SceneNode>> m_scene;
		uint64_t m_idIncreemntal = 0;
	};
}
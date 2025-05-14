#include "stdafx.h"
#pragma once
#include "SceneNode.h"

#include "../mesh/CPUMesh.h"
#include "../mesh/GPUMesh.h"


#include "../Device.h"


namespace Engine {
	class Scene {
	public:
		uint64_t addNode(std::shared_ptr<SceneNode> sceneNode) noexcept {
			m_scene.push_back(sceneNode);
			return m_idIncreemntal++;
		}

		void draw(ID3D12GraphicsCommandList* commandList, Camera* camera, const std::function<bool(CPUMesh&, GPUMesh&, SceneNode* node)>& callback) {
			handleSceneNodesShouldUpdate();
			for (auto& sceneNode : m_scene) {
				sceneNode->draw(commandList, camera, callback);
			}
		}

	private:
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
		std::vector<std::shared_ptr<SceneNode>> m_frustrum;
		uint64_t m_idIncreemntal = 0;
	};
}
#include "stdafx.h"

#pragma once

#include "../../ecs/components/ComponentMesh.h"
#include "AssetStructures.h"



namespace Engine::Scene {
	class AssetManager {
	public:
		AssetManager() {
			m_meshAssetMap.reserve(2ULL << 10);
			m_materialAssetMap.reserve(2ULL << 4);
			m_materialInstanceAssetMap.reserve(2ULL << 10);
		}

        Asset::MeshId registerMesh(Asset::UsageMesh usage, Asset::SourceMesh source, Asset::MeshSourceData sourceData) {
            auto id = generateMeshAssetId();
            Asset::MeshMapValue meshMapValue;
            meshMapValue.usage = usage;
            meshMapValue.source = source;
            meshMapValue.sourceData = sourceData;

            m_meshAssetMap[id] = std::move(meshMapValue);
            Asset::MeshAssetEvent event{};
            event.id = id;
            event.oldStatus = Asset::Status::Unknown;
            event.newStatus = Asset::Status::Unknown;
            event.type = Asset::IAssetEvent::Type::Registered;
            notifyMesh(event);

            return id;
        }

        Asset::MaterialId registerMaterial(Asset::UsageMaterial usage, Asset::SourceMaterial source, Asset::MaterialSourceData sourceData) {
            auto id = generateMaterialAssetId();
            Asset::MaterialMapValue meshMaterialValue;
            meshMaterialValue.usage = usage;
            meshMaterialValue.source = source;
            meshMaterialValue.sourceData = sourceData;

            m_materialAssetMap[id] = std::move(meshMaterialValue);
            
            Asset::MaterialAssetEvent event{};
            event.id = id;
            event.oldStatus = Asset::Status::Unknown;
            event.newStatus = Asset::Status::Unknown;
            event.type = Asset::IAssetEvent::Type::Registered;
            notifyMaterial(event);

            return id;
        }

        Asset::MaterialInstanceId registerMaterialInstance(Asset::MaterialId materialId, Asset::MaterialSourceData sourceData) {
            if (!m_materialAssetMap.contains(materialId)) {
                throw std::runtime_error("[AssetManager] No materialId");
            }
            auto id = generateMaterialInstanceAssetId();
            Asset::MaterialInstanceMapValue materialInstanceValue;
            materialInstanceValue.sourceData = sourceData;

            m_materialInstanceAssetMap[id] = std::move(materialInstanceValue);

            Asset::MaterialInstanceAssetEvent event{};
            event.id = id;
            event.oldStatus = Asset::Status::Unknown;
            event.newStatus = Asset::Status::Unknown;
            event.type = Asset::IAssetEvent::Type::Registered;
            notifyMaterialInstance(event);

            return id;
        }

        Asset::MeshMapValue& getMeshAsset(Asset::MeshId id) {
            return m_meshAssetMap[id];
        }
        Asset::MaterialMapValue& getMaterialAsset(Asset::MaterialId id) {
            return m_materialAssetMap[id];
        }
        Asset::MaterialInstanceMapValue& getMaterialInstanceAsset(Asset::MaterialInstanceId id) {
            return m_materialInstanceAssetMap[id];
        }

        void subscribeMesh(Asset::MeshAssetEventCallback callback) {
            m_meshSubscribers.push_back(std::move(callback));
        }
        void subscribeMaterial(Asset::MaterialAssetEventCallback callback) {
            m_materialSubscribers.push_back(std::move(callback));
        }
        void subscribeMaterialInstance(Asset::MaterialInstanceAssetEventCallback callback) {
            m_materialInstanceSubscribers.push_back(std::move(callback));
        }

        void setMeshStatus(Asset::MeshId id, Asset::Status status) {
            m_meshAssetMap[id].status.store(status, std::memory_order_release);
        }
        void setMaterialStatus(Asset::MaterialId id, Asset::Status status) {
            m_materialAssetMap[id].status.store(status, std::memory_order_release);
        }
        void setMaterialInstanceStatus(Asset::MaterialInstanceId id, Asset::Status status) {
            m_materialInstanceAssetMap[id].status.store(status, std::memory_order_release);
        }

        void notifyMesh(const Asset::MeshAssetEvent& event) {
            for (auto& callback : m_meshSubscribers) {
                callback(event);
            }
        }
        void notifyMaterial(const Asset::MaterialAssetEvent& event) {
            for (auto& callback : m_materialSubscribers) {
                callback(event);
            }
        }
        void notifyMaterialInstance(const Asset::MaterialInstanceAssetEvent& event) {
            for (auto& callback : m_materialInstanceSubscribers) {
                callback(event);
            }
        }
	private:
        Asset::MeshId generateMeshAssetId() {
            return m_nextMeshAssetId.fetch_add(1, std::memory_order_relaxed);
        }

        Asset::MaterialId generateMaterialAssetId() {
            return m_nextMaterialAssetId.fetch_add(1, std::memory_order_relaxed);
        }

        Asset::MaterialInstanceId generateMaterialInstanceAssetId() {
            return m_nextMaterialInstanceAssetId.fetch_add(1, std::memory_order_relaxed);
        }

		ankerl::unordered_dense::map<Asset::MeshId, Asset::MeshMapValue> m_meshAssetMap;
		ankerl::unordered_dense::map<Asset::MaterialId, Asset::MaterialMapValue> m_materialAssetMap;
		ankerl::unordered_dense::map<Asset::MaterialInstanceId, Asset::MaterialInstanceMapValue> m_materialInstanceAssetMap;

        std::vector<Asset::MeshAssetEventCallback> m_meshSubscribers;
        std::vector<Asset::MaterialAssetEventCallback> m_materialSubscribers;
        std::vector<Asset::MaterialInstanceAssetEventCallback> m_materialInstanceSubscribers;

		std::atomic<Asset::MeshId> m_nextMeshAssetId{ 1 };
		std::atomic<Asset::MaterialId> m_nextMaterialAssetId{ 1 };
		std::atomic<Asset::MaterialInstanceId> m_nextMaterialInstanceAssetId{ 1 };
	};
}

#include "stdafx.h"

#pragma once

#include "mesh/AssetMesh.h"
#include "material/AssetMaterial.h"

namespace Engine::Scene::Asset {
	enum class Type {
		Mesh,
		Material,
		MaterialInstance,
	};

	enum class Status {
		Unknown,            // Asset ID exists, but nothing else known yet.
		Queued,             // Scheduled for loading, not started yet.
		MetadataLoaded,     // Metadata parsed (AABB, topology, structure), but no heavy data.
		Loading,            // Currently loading full data (e.g. disk read, network).
		Loaded,             // Loaded from source (CPU memory).
		Initializing,       // Preprocessing, GPU upload, etc.
		Ready,              // Fully initialized and usable.
		Unloaded,           // Removed from memory.
		Error               // Failed to load.
	};

	enum class SourceMesh {
		File,
		Procedural,
	};

	struct FileSourceMesh {
		std::filesystem::path path;
	};
	struct ProceduralSourceMesh {
		//
	};
	using MeshSourceData = std::variant<FileSourceMesh, ProceduralSourceMesh>;

	enum class UsageMesh {
		Static,         // Will not change. Can go in large, contiguous GPU heap.
		Skinned,        // Animated/skinned mesh — needs frequent updates, use fast-access memory.
		Dynamic,        // Procedural or runtime-created, may change frequently.
		EditorOnly,     // Used only in the editor, not shipped in release builds.
		Transient       // Short-lived mesh, e.g., explosion debris.
	};

	enum class SourceMaterial {
		File,
		Procedural,
		EngineInternal,
	};
	struct FileSourceMaterial {
		std::filesystem::path path;
	};
	struct ProceduralSourceMaterial {
		//
	};
	using MaterialSourceData = std::variant<FileSourceMaterial, ProceduralSourceMaterial>;

	enum class UsageMaterial {
		Default,         // Standard deferred material
		Transparent,     // Forward-rendered transparent material
		Unlit,           // Forward-rendered unlit or emissive
		PostProcess,     // Fullscreen material, separate pass
		Transient,       // Temporary (e.g., FX, runtime-only)
		EditorOnly       // Editor visualization/debug
	};

	using MaterialId = uint64_t;
	using MaterialInstanceId = uint64_t;
	using MeshId = uint64_t;

	struct IStatus {
		IStatus(const IStatus&) = delete;
		IStatus& operator=(const IStatus&) = delete;
		IStatus() = default;
		IStatus(IStatus&& other) noexcept
			: status(other.status.load(std::memory_order_acquire)) {
		}
		IStatus& operator=(IStatus&& other) noexcept {
			if (this != &other) {
				status.store(other.status.load(std::memory_order_acquire), std::memory_order_release);
			}
			return *this;
		}

		std::atomic<Status> status{ Status::Unknown };
	};

	struct MeshMapValue : public IStatus {
		Type type = Type::Mesh;
		UsageMesh usage;
		SourceMesh source;
		Mesh asset;

		MeshSourceData sourceData;
	};

	struct MaterialMapValue : public IStatus {
		Type type = Type::Material;
		UsageMaterial usage;
		SourceMaterial source;
		Material asset;


		MaterialSourceData sourceData;
	};

	struct MaterialInstanceMapValue : public IStatus {
		Type type = Type::MaterialInstance;
		MaterialId materialId = 0;
		MaterialInstance asset;

		MaterialSourceData sourceData;
	};

	struct IAssetEvent {
		enum class Type { Registered, StatusChanged, MetadataLoaded, Uploaded } type;
		Status oldStatus;
		Status newStatus;
	};
	struct MeshAssetEvent : public IAssetEvent {
		MeshId id;
	};
	struct MaterialAssetEvent : public IAssetEvent {
		MaterialId id;
	};
	struct MaterialInstanceAssetEvent : public IAssetEvent {
		MaterialInstanceId id;
	};

	using MeshAssetEventCallback = std::function<void(const MeshAssetEvent&)>;
	using MaterialAssetEventCallback = std::function<void(const MaterialAssetEvent&)>;
	using MaterialInstanceAssetEventCallback = std::function<void(const MaterialInstanceAssetEvent&)>;
}

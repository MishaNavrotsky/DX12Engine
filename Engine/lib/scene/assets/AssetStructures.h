#include "stdafx.h"

#pragma once

#include "mesh/AssetMesh.h"
#include "material/AssetMaterial.h"

namespace Engine::Scene::Asset {
	enum class AssetType {
		Mesh,
		Material,
		MaterialInstance,
	};

	enum class AssetStatus {
		Unknown,            // Default state; asset ID exists but we know nothing else yet.
		Queued,             // Scheduled for loading/streaming but not started yet.
		Loading,            // Currently being loaded/streamed (e.g., async read from disk/network).
		Loaded,             // Successfully loaded and ready to use.
		Initializing,       // Loaded, but still processing (e.g., GPU upload or baking).
		Ready,              // Fully initialized and usable (GPU-bound, runtime ready).
		Unloaded,			// Removed from memory
		Error,              // Failed to load (corrupted, missing, etc.).
	};

	enum class AssetSourceMesh {
		File,
		Procedural,
	};

	enum class AssetUsageMesh {
		Static,         // Will not change. Can go in large, contiguous GPU heap.
		Skinned,        // Animated/skinned mesh — needs frequent updates, use fast-access memory.
		Dynamic,        // Procedural or runtime-created, may change frequently.
		EditorOnly,     // Used only in the editor, not shipped in release builds.
		Transient       // Short-lived mesh, e.g., explosion debris.
	};

	enum class AssetSourceMaterial {
		File,
		Procedural,
		EngineInternal,
	};

	enum class AssetUsageMaterial {
		Default,         // Standard deferred material
		Transparent,     // Forward-rendered transparent material
		Unlit,           // Forward-rendered unlit or emissive
		PostProcess,     // Fullscreen material, separate pass
		Transient,       // Temporary (e.g., FX, runtime-only)
		EditorOnly       // Editor visualization/debug
	};

	using MaterialAssetId = uint64_t;
	using MaterialInstanceAssetId = uint64_t;
	using MeshAssetId = uint64_t;

	struct MeshAssetMapValue {
		AssetType type = Asset::AssetType::Mesh;
		AssetStatus status = Asset::AssetStatus::Unknown;
		AssetMesh asset;
	};

	struct MaterialAssetMapValue {
		AssetType type = Asset::AssetType::Material;
		AssetStatus status = Asset::AssetStatus::Unknown;
		AssetMaterial asset;
	};

	struct MaterialInstanceAssetMapValue {
		AssetType type = Asset::AssetType::MaterialInstance;
		AssetStatus status = Asset::AssetStatus::Unknown;
		MaterialAssetId materialId = 0;
		AssetMaterialInstance asset;
	};
}

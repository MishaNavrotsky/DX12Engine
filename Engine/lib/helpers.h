#include "stdafx.h"

#pragma once

#include "mesh/CPUMesh.h"
#include "camera/Camera.h"
#include "managers/CPUMaterialManager.h"
#include "managers/GPUMaterialManager.h"
#include "managers/CPUMeshManager.h"
#include "managers/GPUMeshManager.h"
#include "managers/CPUTextureManager.h"
#include "managers/GPUTextureManager.h"
#include "managers/SamplerManager.h"
#include "managers/ModelHeapsManager.h"
#include "managers/ModelManager.h"


namespace Engine::Helpers {
	inline void fullyDeleteModel(GUID modelGUID) {
		static auto& modelManager = ModelManager::GetInstance();
		static auto& cpuMeshManager = CPUMeshManager::GetInstance();
		static auto& cpuMaterialManager = CPUMaterialManager::GetInstance();
		static auto& cpuTextureManager = CPUTextureManager::GetInstance();
		static auto& gpuMeshManager = GPUMeshManager::GetInstance();
		static auto& gpuMaterialManager = GPUMaterialManager::GetInstance();
		static auto& gpuTextureManager = GPUTextureManager::GetInstance();
		static auto& samplerManager = SamplerManager::GetInstance();
		static auto& modelHeapsManager = ModelHeapsManager::GetInstance();
	}

	template<size_t N>
	inline std::vector<float> FlattenXMFLOAT3Array(const std::array<DirectX::XMFLOAT3, N>& arr) {
		std::vector<float> flat;
		flat.reserve(N * 3);
		for (const auto& v : arr) {
			flat.push_back(v.x);
			flat.push_back(v.y);
			flat.push_back(v.z);
		}
		return flat;
	}

	inline std::vector<float> FlattenXMFLOAT3Vector(const std::vector<DirectX::XMFLOAT3>& vec) {
		std::vector<float> flat;
		flat.reserve(vec.size() * 3);
		for (const auto& v : vec) {
			flat.push_back(v.x);
			flat.push_back(v.y);
			flat.push_back(v.z);
		}
		return flat;
	}

	inline void TransformAABB_ObjectToWorld(const AABB& objectAABB, const XMMATRIX& worldMatrix, AABB& aabb) {
		using namespace DirectX;
		// Load min and max
		XMVECTOR minV = objectAABB.min;
		XMVECTOR maxV = objectAABB.max;

		// Generate all 8 corners of the AABB
		const XMVECTOR corners[8] = {
			XMVectorSet(minV.m128_f32[0], minV.m128_f32[1], minV.m128_f32[2], 1.0f),
			XMVectorSet(minV.m128_f32[0], minV.m128_f32[1], maxV.m128_f32[2], 1.0f),
			XMVectorSet(minV.m128_f32[0], maxV.m128_f32[1], minV.m128_f32[2], 1.0f),
			XMVectorSet(minV.m128_f32[0], maxV.m128_f32[1], maxV.m128_f32[2], 1.0f),
			XMVectorSet(maxV.m128_f32[0], minV.m128_f32[1], minV.m128_f32[2], 1.0f),
			XMVectorSet(maxV.m128_f32[0], minV.m128_f32[1], maxV.m128_f32[2], 1.0f),
			XMVectorSet(maxV.m128_f32[0], maxV.m128_f32[1], minV.m128_f32[2], 1.0f),
			XMVectorSet(maxV.m128_f32[0], maxV.m128_f32[1], maxV.m128_f32[2], 1.0f),
		};

		// Initialize new min/max
		XMVECTOR transformedMin = XMVectorReplicate(std::numeric_limits<float>::max());
		XMVECTOR transformedMax = XMVectorReplicate(std::numeric_limits<float>::lowest());

		// Transform each corner and expand bounds
		for (int i = 0; i < 8; ++i) {
			XMVECTOR transformed = XMVector3Transform(corners[i], worldMatrix);
			transformedMin = XMVectorMin(transformedMin, transformed);
			transformedMax = XMVectorMax(transformedMax, transformed);
		}

		aabb.min = transformedMin;
		aabb.max = transformedMax;
	}

	inline bool AABBInFrustum(const XMVECTOR* frustumPlanes, const XMFLOAT3& minBounds, const XMFLOAT3& maxBounds)
	{
		for (int i = 0; i < 6; i++)
		{
			XMVECTOR plane = frustumPlanes[i];

			// Select the most positive vertex for the plane test
			XMFLOAT3 positiveVertex = {
				(XMVectorGetX(plane) >= 0) ? maxBounds.x : minBounds.x,
				(XMVectorGetY(plane) >= 0) ? maxBounds.y : minBounds.y,
				(XMVectorGetZ(plane) >= 0) ? maxBounds.z : minBounds.z
			};

			// Perform the plane test
			if (XMVectorGetX(plane) * positiveVertex.x +
				XMVectorGetY(plane) * positiveVertex.y +
				XMVectorGetZ(plane) * positiveVertex.z +
				XMVectorGetW(plane) < 0)
			{
				return false; // AABB is outside the frustum
			}
		}
		return true; // AABB is inside or intersecting the frustum
	}

	inline bool AABBInFrustum(const XMVECTOR* frustumPlanes, const XMVECTOR minBounds, const XMVECTOR maxBounds)
	{
		for (int i = 0; i < 6; i++)
		{
			XMVECTOR plane = frustumPlanes[i];

			// Select the most positive vertex for the plane test
			XMVECTOR positiveVertex = XMVectorSet(
				(XMVectorGetX(plane) >= 0) ? XMVectorGetX(maxBounds) : XMVectorGetX(minBounds),
				(XMVectorGetY(plane) >= 0) ? XMVectorGetY(maxBounds) : XMVectorGetY(minBounds),
				(XMVectorGetZ(plane) >= 0) ? XMVectorGetZ(maxBounds) : XMVectorGetZ(minBounds),
				1.0f // Homogeneous coordinate
			);

			// Perform the plane test
			if (XMVectorGetX(plane) * XMVectorGetX(positiveVertex) +
				XMVectorGetY(plane) * XMVectorGetY(positiveVertex) +
				XMVectorGetZ(plane) * XMVectorGetZ(positiveVertex) +
				XMVectorGetW(plane) < 0)
			{
				return false; // AABB is outside the frustum
			}
		}
		return true; // AABB is inside or intersecting the frustum
	}
}



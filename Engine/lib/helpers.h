#include "stdafx.h"

#pragma once

#include "mesh/CPUMesh.h"
#include "camera/Camera.h"

namespace Engine::Helpers {
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

	inline bool IsAABBInFrustum(const Frustum& frustum, const XMVECTOR& minBounds, const XMVECTOR& maxBounds) {
		for (int i = 0; i < 6; i++) {
			XMVECTOR plane = frustum.planes[i];

			// Extract the normal (ignore the .w component)
			XMVECTOR normal = XMVectorSetW(plane, 0.0f);

			// Compute the positive vertex (furthest in the direction of the plane normal)
			XMVECTOR positiveVertex = XMVectorSelect(minBounds, maxBounds, XMVectorGreaterOrEqual(normal, XMVectorZero()));

			// If the positive vertex is outside the plane, the AABB is outside the frustum
			if (XMVectorGetX(XMPlaneDotCoord(plane, positiveVertex)) < 0.0f) {
				return false;
			}
		}

		return true; // AABB is inside or intersects the frustum
	}
}



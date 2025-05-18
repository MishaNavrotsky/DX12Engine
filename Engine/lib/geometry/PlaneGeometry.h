#include "stdafx.h"

#pragma once

#include <vector>
#include <DirectXMath.h>

namespace Engine::Geometry {
	struct PlaneGeometry {
		std::vector<DirectX::XMFLOAT3> vertices;
		std::vector<unsigned int> indices;
		std::vector<DirectX::XMFLOAT3> normals;
	};

	// Generates a plane centered at the origin, lying on the XZ plane
	inline PlaneGeometry GeneratePlane(float width, float depth, int xSegments, int ySegments) {
		PlaneGeometry plane;

		using namespace DirectX;

		float halfWidth = width * 0.5f;
		float halfDepth = depth * 0.5f;

		// Step size between vertices
		float dx = width / xSegments;
		float dy = depth / ySegments;

		// Generate vertices and normals
		for (int y = 0; y <= ySegments; ++y) {
			for (int x = 0; x <= xSegments; ++x) {
				float xPos = -halfWidth + x * dx;
				float yPos = -halfDepth + y * dy;
				plane.vertices.emplace_back(XMFLOAT3(xPos, yPos, 0.0f));
				plane.normals.emplace_back(XMFLOAT3(0.0f, 0.0f, 1.0f)); // Upward normal
			}
		}

		// Generate indices (two triangles per quad)
		for (int y = 0; y < ySegments; ++y) {
			for (int x = 0; x < xSegments; ++x) {
				int row1 = y * (xSegments + 1);
				int row2 = (y + 1) * (xSegments + 1);

				int i0 = row1 + x;
				int i1 = row2 + x;
				int i2 = row1 + x + 1;
				int i3 = row2 + x + 1;

				// Triangle 1
				plane.indices.push_back(i0);
				plane.indices.push_back(i1);
				plane.indices.push_back(i2);

				// Triangle 2
				plane.indices.push_back(i2);
				plane.indices.push_back(i1);
				plane.indices.push_back(i3);
			}
		}

		return plane;
	}

	inline std::array<DirectX::XMFLOAT3, 4> GeneratePlaneQuad(const DirectX::XMVECTOR& plane) {
		DirectX::XMFLOAT4 planeData;
		XMStoreFloat4(&planeData, plane);

		DirectX::XMVECTOR normal = XMVectorSet(planeData.x, planeData.y, planeData.z, 0.0f);
		float d = planeData.w;

		// Find a reference point on the plane
		DirectX::XMVECTOR center = XMVectorScale(normal, -d);

		// Generate two perpendicular vectors to form the quad basis
		DirectX::XMVECTOR right = XMVector3Normalize(XMVectorSet(planeData.y, -planeData.x, 0.0f, 0.0f));
		DirectX::XMVECTOR up = XMVector3Normalize(XMVector3Cross(normal, right));

		// Define quad corners
		DirectX::XMVECTOR p1 = XMVectorAdd(center, XMVectorScale(right, 5.0f));
		DirectX::XMVECTOR p2 = XMVectorAdd(center, XMVectorScale(up, 5.0f));
		DirectX::XMVECTOR p3 = XMVectorSubtract(center, XMVectorScale(right, 5.0f));
		DirectX::XMVECTOR p4 = XMVectorSubtract(center, XMVectorScale(up, 5.0f));

		// Store result
		std::array<DirectX::XMFLOAT3, 4> quad;
		XMStoreFloat3(&quad[0], p1);
		XMStoreFloat3(&quad[1], p2);
		XMStoreFloat3(&quad[2], p3);
		XMStoreFloat3(&quad[3], p4);

		return quad;
	}
}


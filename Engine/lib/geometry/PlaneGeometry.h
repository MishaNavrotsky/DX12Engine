#include "stdafx.h"

#pragma once

#include <vector>
#include <DirectXMath.h>

namespace Engine::Geometry {
	struct PlaneGeometry {
		std::vector<DX::XMFLOAT3> vertices;
		std::vector<unsigned int> indices;
		std::vector<DX::XMFLOAT3> normals;
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

	inline std::array<DX::XMFLOAT3, 4> GeneratePlaneQuad(const DX::XMVECTOR& plane) {
		DX::XMFLOAT4 planeData;
		XMStoreFloat4(&planeData, plane);

		DX::XMVECTOR normal = DX::XMVectorSet(planeData.x, planeData.y, planeData.z, 0.0f);
		float d = planeData.w;

		// Find a reference point on the plane
		DX::XMVECTOR center = DX::XMVectorScale(normal, -d);

		// Generate two perpendicular vectors to form the quad basis
		DX::XMVECTOR right = DX::XMVector3Normalize(DX::XMVectorSet(planeData.y, -planeData.x, 0.0f, 0.0f));
		DX::XMVECTOR up = DX::XMVector3Normalize(DX::XMVector3Cross(normal, right));

		// Define quad corners
		DX::XMVECTOR p1 = DX::XMVectorAdd(center, DX::XMVectorScale(right, 5.0f));
		DX::XMVECTOR p2 = DX::XMVectorAdd(center, DX::XMVectorScale(up, 5.0f));
		DX::XMVECTOR p3 = DX::XMVectorSubtract(center, DX::XMVectorScale(right, 5.0f));
		DX::XMVECTOR p4 = DX::XMVectorSubtract(center, DX::XMVectorScale(up, 5.0f));

		// Store result
		std::array<DX::XMFLOAT3, 4> quad;
		DX::XMStoreFloat3(&quad[0], p1);
		DX::XMStoreFloat3(&quad[1], p2);
		DX::XMStoreFloat3(&quad[2], p3);
		DX::XMStoreFloat3(&quad[3], p4);

		return quad;
	}
}


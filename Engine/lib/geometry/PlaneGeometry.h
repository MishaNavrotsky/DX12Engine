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
}


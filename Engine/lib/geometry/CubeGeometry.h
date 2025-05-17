#include "stdafx.h"
#pragma once
#include <array>
#include <DirectXMath.h>


namespace Engine::Geometry {
	struct CubeGeometry {
		std::array<DirectX::XMFLOAT3, 8> vertices;
		std::array<uint32_t, 36> indices;
		std::array<uint32_t, 24> lineListIndices;

		std::array<DirectX::XMFLOAT3, 6> normals;
	};

	inline CubeGeometry GenerateCube(const std::array<DirectX::XMFLOAT3, 8>& inputVertices) {
		CubeGeometry cube;
		cube.vertices = inputVertices;

		// Define indices for 12 triangles (6 faces)
		cube.indices = {
			0, 1, 2, 2, 3, 0, // Front
			4, 5, 6, 6, 7, 4, // Back
			0, 4, 7, 7, 3, 0, // Left
			1, 5, 6, 6, 2, 1, // Right
			3, 2, 6, 6, 7, 3, // Top
			0, 1, 5, 5, 4, 0  // Bottom
		};

		cube.lineListIndices = {
			//0, 1, 1, 2, 2, 3, 3, 0, // Front face
			//4, 5, 5, 6, 6, 7, 7, 4, // Back face
			//0, 4, 1, 5, 2, 6, 3, 7  // Connecting edges
			// Edges of the cube
			0, 1, // v0 to v1
			1, 2, // v1 to v2
			2, 3, // v2 to v3
			3, 0, // v3 to v0
			4, 5, // v4 to v5
			5, 6, // v5 to v6
			6, 7, // v6 to v7
			7, 4, // v7 to v4
			0, 4, // v0 to v4
			1, 5, // v1 to v5
			2, 6, // v2 to v6
			3, 7  // v3 to v7
		};

		// Define normals for basic lighting
		cube.normals = {
			DirectX::XMFLOAT3(0.0f,  0.0f,  1.0f),  // Front
			DirectX::XMFLOAT3(0.0f,  0.0f, -1.0f),  // Back
			DirectX::XMFLOAT3(-1.0f,  0.0f,  0.0f),  // Left
			DirectX::XMFLOAT3(1.0f,  0.0f,  0.0f),  // Right
			DirectX::XMFLOAT3(0.0f,  1.0f,  0.0f),  // Top
			DirectX::XMFLOAT3(0.0f, -1.0f,  0.0f)   // Bottom
		};

		return cube;
	}

	inline CubeGeometry GenerateCubeFromPoints(const DirectX::XMFLOAT3& minPoint, const DirectX::XMFLOAT3& maxPoint) {
		// Compute 8 cube vertices from min & max bounds
		std::array<DirectX::XMFLOAT3, 8> vertices = {
			DirectX::XMFLOAT3(minPoint.x, minPoint.y, minPoint.z), // v0
			DirectX::XMFLOAT3(maxPoint.x, minPoint.y, minPoint.z), // v1
			DirectX::XMFLOAT3(maxPoint.x, minPoint.y, maxPoint.z), // v2
			DirectX::XMFLOAT3(minPoint.x, minPoint.y, maxPoint.z), // v3
			DirectX::XMFLOAT3(minPoint.x, maxPoint.y, minPoint.z), // v4
			DirectX::XMFLOAT3(maxPoint.x, maxPoint.y, minPoint.z), // v5
			DirectX::XMFLOAT3(maxPoint.x, maxPoint.y, maxPoint.z), // v6
			DirectX::XMFLOAT3(minPoint.x, maxPoint.y, maxPoint.z)  // v7
		};

		return GenerateCube(vertices);
	}
}
#include "stdafx.h"

#pragma once

#include <vector>
#include <DirectXMath.h>

struct ConeGeometry {
    std::vector<DX::XMFLOAT3> vertices;
    std::vector<unsigned int> indices;
    std::vector<DX::XMFLOAT3> normals;
};

// Generates a cone aligned along the Y axis with its base centered at the origin
ConeGeometry GenerateCone(float radius, float height, int segments) {
    ConeGeometry cone;

    using namespace DirectX;

    // Tip of the cone
    XMFLOAT3 tip(0.0f, height, 0.0f);
    cone.vertices.push_back(tip);
    cone.normals.push_back(XMFLOAT3(0.0f, 1.0f, 0.0f)); // Placeholder; will be updated if needed

    // Generate base circle vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = i * XM_2PI / segments;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);
        cone.vertices.push_back(XMFLOAT3(x, 0.0f, z));

        // Compute normal for the side (approximate)
        XMVECTOR edge = XMVector3Normalize(XMVectorSet(x, height, z, 0.0f));
        XMFLOAT3 normal;
        XMStoreFloat3(&normal, edge);
        cone.normals.push_back(normal);
    }

    // Side triangles
    for (int i = 1; i <= segments; ++i) {
        cone.indices.push_back(0);         // tip
        cone.indices.push_back(i);         // current base point
        cone.indices.push_back(i + 1);     // next base point
    }

    // Center of base
    XMFLOAT3 baseCenter(0.0f, 0.0f, 0.0f);
    int baseCenterIndex = static_cast<int>(cone.vertices.size());
    cone.vertices.push_back(baseCenter);
    cone.normals.push_back(XMFLOAT3(0.0f, -1.0f, 0.0f));

    // Base triangles
    for (int i = 1; i <= segments; ++i) {
        cone.indices.push_back(baseCenterIndex);
        cone.indices.push_back(i + 1);
        cone.indices.push_back(i);
    }

    return cone;
}


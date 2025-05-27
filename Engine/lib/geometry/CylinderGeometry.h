#include "stdafx.h"

#pragma once

#include <vector>
#include <DirectXMath.h>

struct CylinderGeometry {
    std::vector<DX::XMFLOAT3> vertices;
    std::vector<unsigned int> indices;
    std::vector<DX::XMFLOAT3> normals;
};

CylinderGeometry GenerateCylinder(float radius, float height, int segments) {
    CylinderGeometry cyl;

    using namespace DirectX;

    float halfHeight = height / 2.0f;

    // Side vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = XM_2PI * i / segments;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);

        // Bottom vertex
        cyl.vertices.emplace_back(XMFLOAT3(x, -halfHeight, z));
        cyl.normals.emplace_back(XMFLOAT3(x, 0.0f, z)); // Approximate normal

        // Top vertex
        cyl.vertices.emplace_back(XMFLOAT3(x, +halfHeight, z));
        cyl.normals.emplace_back(XMFLOAT3(x, 0.0f, z));
    }

    // Side indices (2 triangles per segment)
    for (int i = 0; i < segments; ++i) {
        int i0 = i * 2;
        int i1 = i0 + 1;
        int i2 = i0 + 2;
        int i3 = i0 + 3;

        // First triangle
        cyl.indices.push_back(i0);
        cyl.indices.push_back(i2);
        cyl.indices.push_back(i1);

        // Second triangle
        cyl.indices.push_back(i1);
        cyl.indices.push_back(i2);
        cyl.indices.push_back(i3);
    }

    // Top and bottom center vertices
    int bottomCenterIdx = static_cast<int>(cyl.vertices.size());
    cyl.vertices.emplace_back(XMFLOAT3(0.0f, -halfHeight, 0.0f));
    cyl.normals.emplace_back(XMFLOAT3(0.0f, -1.0f, 0.0f));

    int topCenterIdx = static_cast<int>(cyl.vertices.size());
    cyl.vertices.emplace_back(XMFLOAT3(0.0f, +halfHeight, 0.0f));
    cyl.normals.emplace_back(XMFLOAT3(0.0f, +1.0f, 0.0f));

    // Cap indices
    for (int i = 0; i < segments; ++i) {
        int base = i * 2;

        // Bottom cap
        cyl.indices.push_back(bottomCenterIdx);
        cyl.indices.push_back(base + 2);
        cyl.indices.push_back(base);

        // Top cap
        cyl.indices.push_back(topCenterIdx);
        cyl.indices.push_back(base + 1);
        cyl.indices.push_back(base + 3);
    }

    return cyl;
}


#include "stdafx.h"

#pragma once

#include <vector>
#include <DirectXMath.h>

struct ArrowGeometry {
    std::vector<DirectX::XMFLOAT3> vertices;
    std::vector<unsigned int> indices;
    std::vector<DirectX::XMFLOAT3> normals;
};

// Combines a cylinder (shaft) and a cone (head) to form an arrow
ArrowGeometry GenerateArrow(float shaftRadius, float shaftLength, float headRadius, float headLength, int segments) {
    ArrowGeometry arrow;

    using namespace DirectX;

    int baseVertexIndex = 0;

    // 1. Shaft (cylinder)
    float halfShaft = shaftLength / 2.0f;
    for (int i = 0; i <= segments; ++i) {
        float angle = XM_2PI * i / segments;
        float x = cosf(angle);
        float z = sinf(angle);

        XMFLOAT3 normal(x, 0.0f, z);

        // Bottom ring
        arrow.vertices.emplace_back(XMFLOAT3(x * shaftRadius, 0.0f, z * shaftRadius));
        arrow.normals.push_back(normal);

        // Top ring
        arrow.vertices.emplace_back(XMFLOAT3(x * shaftRadius, shaftLength, z * shaftRadius));
        arrow.normals.push_back(normal);
    }

    // Indices for shaft
    for (int i = 0; i < segments; ++i) {
        int i0 = i * 2;
        int i1 = i0 + 1;
        int i2 = i0 + 2;
        int i3 = i0 + 3;

        arrow.indices.push_back(i0);
        arrow.indices.push_back(i2);
        arrow.indices.push_back(i1);

        arrow.indices.push_back(i1);
        arrow.indices.push_back(i2);
        arrow.indices.push_back(i3);
    }

    // 2. Head (cone)
    int headBaseStart = static_cast<int>(arrow.vertices.size());
    XMFLOAT3 coneTip(0.0f, shaftLength + headLength, 0.0f);
    arrow.vertices.push_back(coneTip); // tip
    arrow.normals.push_back(XMFLOAT3(0.0f, 1.0f, 0.0f));

    for (int i = 0; i <= segments; ++i) {
        float angle = XM_2PI * i / segments;
        float x = cosf(angle);
        float z = sinf(angle);
        arrow.vertices.emplace_back(XMFLOAT3(x * headRadius, shaftLength, z * headRadius));

        // Side normal (approximate)
        XMFLOAT3 normal(x, 0.5f, z); // tilted slightly outward
        arrow.normals.push_back(normal);
    }

    // Indices for cone
    for (int i = 1; i <= segments; ++i) {
        arrow.indices.push_back(headBaseStart);         // tip
        arrow.indices.push_back(headBaseStart + i);
        arrow.indices.push_back(headBaseStart + i + 1);
    }

    return arrow;
}

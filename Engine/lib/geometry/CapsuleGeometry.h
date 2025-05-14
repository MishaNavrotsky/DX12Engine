#include "stdafx.h"

#pragma once

#include <vector>
#include <DirectXMath.h>

struct CapsuleGeometry {
    std::vector<DirectX::XMFLOAT3> vertices;
    std::vector<unsigned int> indices;
    std::vector<DirectX::XMFLOAT3> normals;
};

CapsuleGeometry GenerateCapsule(float radius, float height, int latitudeSegments, int longitudeSegments) {
    CapsuleGeometry cap;

    using namespace DirectX;

    float cylinderHalfHeight = height / 2.0f;

    // Hemisphere + Cylinder side
    for (int lat = 0; lat <= latitudeSegments * 2; ++lat) {
        float theta = XM_PI * lat / (latitudeSegments * 2);
        float sinTheta = sinf(theta);
        float cosTheta = cosf(theta);

        float y = radius * cosTheta;

        for (int lon = 0; lon <= longitudeSegments; ++lon) {
            float phi = XM_2PI * lon / longitudeSegments;
            float x = radius * sinTheta * cosf(phi);
            float z = radius * sinTheta * sinf(phi);

            float yOffset = (lat < latitudeSegments)
                ? cylinderHalfHeight // top hemisphere
                : -cylinderHalfHeight; // bottom hemisphere

            if (lat >= latitudeSegments && lat <= latitudeSegments)
                yOffset = 0.0f; // cylinder part

            cap.vertices.emplace_back(XMFLOAT3(x, y + yOffset, z));
            cap.normals.emplace_back(XMFLOAT3(x, y, z)); // Normal from origin
        }
    }

    // Indices (similar to sphere)
    int vertsPerRow = longitudeSegments + 1;
    for (int lat = 0; lat < latitudeSegments * 2; ++lat) {
        for (int lon = 0; lon < longitudeSegments; ++lon) {
            int i0 = lat * vertsPerRow + lon;
            int i1 = i0 + vertsPerRow;

            cap.indices.push_back(i0);
            cap.indices.push_back(i1);
            cap.indices.push_back(i0 + 1);

            cap.indices.push_back(i0 + 1);
            cap.indices.push_back(i1);
            cap.indices.push_back(i1 + 1);
        }
    }

    return cap;
}

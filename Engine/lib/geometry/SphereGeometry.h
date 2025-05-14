#include "stdafx.h"

#pragma once
#include <vector>
#include <DirectXMath.h>
#include <cmath>

struct SphereGeometry {
    std::vector<DirectX::XMFLOAT3> vertices;
    std::vector<unsigned int> indices;
    std::vector<DirectX::XMFLOAT3> normals;
};

SphereGeometry GenerateSphere(float radius, int latitudeSegments, int longitudeSegments) {
    SphereGeometry sphere;

    using namespace DirectX;

    // Generate vertices
    for (int lat = 0; lat <= latitudeSegments; ++lat) {
        float theta = lat * XM_PI / latitudeSegments;
        float sinTheta = sinf(theta);
        float cosTheta = cosf(theta);

        for (int lon = 0; lon <= longitudeSegments; ++lon) {
            float phi = lon * XM_2PI / longitudeSegments;
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            XMFLOAT3 normal(x, y, z);
            XMFLOAT3 position(x * radius, y * radius, z * radius);

            sphere.vertices.push_back(position);
            sphere.normals.push_back(normal);
        }
    }

    // Generate indices
    for (int lat = 0; lat < latitudeSegments; ++lat) {
        for (int lon = 0; lon < longitudeSegments; ++lon) {
            int first = lat * (longitudeSegments + 1) + lon;
            int second = first + longitudeSegments + 1;

            sphere.indices.push_back(first);
            sphere.indices.push_back(second);
            sphere.indices.push_back(first + 1);

            sphere.indices.push_back(second);
            sphere.indices.push_back(second + 1);
            sphere.indices.push_back(first + 1);
        }
    }

    return sphere;
}
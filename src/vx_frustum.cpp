#include "vx_frustum.hpp"
#include <stdlib.h>
#include <stdbool.h>
#include "um_math.hpp"
#include "vx_chunk_manager.hpp"

bool
vx::Frustum::chunk_inside(const vx::Chunk& chunk) const
{
    Vec3f offsetFarRight = this->right * (0.5 * this->zfarWidth);

    Vec3f farTopLeft = this->zfarCenter + this->up * (this->zfarHeight * 0.5) - offsetFarRight;
    Vec3f farTopRight = this->zfarCenter + this->up * (this->zfarHeight * 0.5) + offsetFarRight;
    Vec3f farBottomLeft = this->zfarCenter - this->up * (this->zfarHeight * 0.5) - offsetFarRight;
    Vec3f farBottomRight = this->zfarCenter - this->up * (this->zfarHeight * 0.5) + offsetFarRight;

    Vec3f offset_near_right = this->right * (this->znearWidth * 0.5);
    Vec3f nearTopLeft =
        (this->znearCenter + this->up * (this->znearHeight * 0.5)) - this->right * (this->znearWidth * 0.5);
    Vec3f nearTopRight =
        (this->znearCenter + this->up * (this->znearHeight * 0.5)) + this->right * (this->znearWidth * 0.5);
    Vec3f nearBottomLeft =
        (this->znearCenter - this->up * (this->znearHeight * 0.5)) - this->right * (this->znearWidth * 0.5);
    Vec3f nearBottomRight =
        (this->znearCenter - this->up * (this->znearHeight * 0.5)) + this->right * (this->znearWidth * 0.5);

    Vec3f topLeftVec = farTopLeft - nearTopLeft;
    Vec3f bottomLeftVec = farBottomLeft - nearBottomLeft;
    Vec3f topRightVec = farTopRight - nearTopRight;
    Vec3f bottomRightVec = farBottomRight - nearBottomRight;
    Vec3f farH = farTopLeft - farTopRight;
    Vec3f farV = farBottomLeft - farTopLeft;
    Vec3f frustumNormals[6] =
    {
        // Right normal
        um::normalize(um::cross(bottomRightVec, topRightVec)),
        /* UM::Normalize(Vec3f_Cross(farV, topRightVec)), */
        // LEft normal
        um::normalize(um::cross(topLeftVec, bottomLeftVec)),
        // TOp normal
        um::normalize(um::cross(topRightVec, topLeftVec)),
        // BOttom normal
        um::normalize(um::cross(bottomLeftVec, bottomRightVec)),
        // FAr normal
        um::normalize(um::cross(farV, farH)),
        // NEar normal
        um::normalize(um::cross(farH, farV))
    };
    Vec3f frustumPoints[6] =
    {
        farTopRight,
        farTopLeft,
        farTopRight,
        farBottomRight,
        farBottomRight,
        nearTopLeft,
    };

    bool insideFrustum = true;
    Vec3f pointVec;
    for (i32 p = 0; p < 6; p++)
    {
        bool insidePlane = false;
        for (i32 v = 0; v < 8; v++)
        {
            pointVec = frustumPoints[p] - chunk.max_vertices[v];
            if (um::dot(pointVec, frustumNormals[p]) >= 0.0f)
            {
                insidePlane = true;
            }
        }
        if (!insidePlane)
        {
            insideFrustum = false;
            break;
        }
    }
    return insideFrustum;
}

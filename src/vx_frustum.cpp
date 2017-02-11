#include "vx_frustum.hpp"
#include <stdlib.h>
#include <stdbool.h>
#include "vx_chunk_manager.hpp"

bool
vx::Frustum::chunk_inside(const vx::Chunk& chunk) const
{
    using vec3 = glm::vec3;

    vec3 offsetFarRight = this->right * (0.5f * this->zfar_width);

    vec3 farTopLeft = this->zfar_center + this->up * (this->zfar_height * 0.5f) - offsetFarRight;
    vec3 farTopRight = this->zfar_center + this->up * (this->zfar_height * 0.5f) + offsetFarRight;
    vec3 farBottomLeft = this->zfar_center - this->up * (this->zfar_height * 0.5f) - offsetFarRight;
    vec3 farBottomRight = this->zfar_center - this->up * (this->zfar_height * 0.5f) + offsetFarRight;

    vec3 offset_near_right = this->right * (this->znear_width * 0.5f);
    vec3 nearTopLeft =
        (this->znear_center + this->up * (this->znear_height * 0.5f)) - this->right * (this->znear_width * 0.5f);
    vec3 nearTopRight =
        (this->znear_center + this->up * (this->znear_height * 0.5f)) + this->right * (this->znear_width * 0.5f);
    vec3 nearBottomLeft =
        (this->znear_center - this->up * (this->znear_height * 0.5f)) - this->right * (this->znear_width * 0.5f);
    vec3 nearBottomRight =
        (this->znear_center - this->up * (this->znear_height * 0.5f)) + this->right * (this->znear_width * 0.5f);

    vec3 topLeftVec = farTopLeft - nearTopLeft;
    vec3 bottomLeftVec = farBottomLeft - nearBottomLeft;
    vec3 topRightVec = farTopRight - nearTopRight;
    vec3 bottomRightVec = farBottomRight - nearBottomRight;
    vec3 farH = farTopLeft - farTopRight;
    vec3 farV = farBottomLeft - farTopLeft;
    vec3 frustumNormals[6] =
    {
        // Right normal
        glm::normalize(glm::cross(bottomRightVec, topRightVec)),
        /* UM::Normalize(Vec3f_Cross(farV, topRightVec)), */
        // LEft normal
        glm::normalize(glm::cross(topLeftVec, bottomLeftVec)),
        // TOp normal
        glm::normalize(glm::cross(topRightVec, topLeftVec)),
        // BOttom normal
        glm::normalize(glm::cross(bottomLeftVec, bottomRightVec)),
        // FAr normal
        glm::normalize(glm::cross(farV, farH)),
        // NEar normal
        glm::normalize(glm::cross(farH, farV))
    };
    vec3 frustumPoints[6] =
    {
        farTopRight,
        farTopLeft,
        farTopRight,
        farBottomRight,
        farBottomRight,
        nearTopLeft,
    };

    bool insideFrustum = true;
    vec3 pointVec;
    for (i32 p = 0; p < 6; p++)
    {
        bool insidePlane = false;
        for (i32 v = 0; v < 8; v++)
        {
            pointVec = frustumPoints[p] - chunk.max_vertices[v];
            if (glm::dot(pointVec, frustumNormals[p]) >= 0.0f)
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

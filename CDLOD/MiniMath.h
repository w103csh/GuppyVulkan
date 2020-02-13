//////////////////////////////////////////////////////////////////////
// Modifications copyright(C) 2020 Colin Hughes<colin.s.hughes @gmail.com>
// -------------------------------
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#ifndef MINI_MATH_H
#define MINI_MATH_H

#include <algorithm>
#include <glm/glm.hpp>

enum IntersectType { IT_Outside, IT_Intersect, IT_Inside };

struct AABB {
    glm::vec3 Min;
    glm::vec3 Max;
    glm::vec3 Center() { return (Min + Max) * 0.5f; }
    glm::vec3 Size() { return Max - Min; }

    AABB() {}
    AABB(const glm::vec3& min, const glm::vec3& max) : Min(min), Max(max) {}

    void GetCornerPoints(glm::vec3 corners[]) {
        corners[0].x = Min.x;
        corners[0].y = Min.y;
        corners[0].z = Min.z;

        corners[1].x = Min.x;
        corners[1].y = Max.y;
        corners[1].z = Min.z;

        corners[2].x = Max.x;
        corners[2].y = Min.y;
        corners[2].z = Min.z;

        corners[3].x = Max.x;
        corners[3].y = Max.y;
        corners[3].z = Min.z;

        corners[4].x = Min.x;
        corners[4].y = Min.y;
        corners[4].z = Max.z;

        corners[5].x = Min.x;
        corners[5].y = Max.y;
        corners[5].z = Max.z;

        corners[6].x = Max.x;
        corners[6].y = Min.y;
        corners[6].z = Max.z;

        corners[7].x = Max.x;
        corners[7].y = Max.y;
        corners[7].z = Max.z;
    }

    bool Intersects(const AABB& other) {
        return !((other.Max.x < this->Min.x) || (other.Min.x > this->Max.x) || (other.Max.y < this->Min.y) ||
                 (other.Min.y > this->Max.y) || (other.Max.z < this->Min.z) || (other.Min.z > this->Max.z));
    }

    IntersectType TestInBoundingPlanes(const glm::vec4 planes[]) {
        glm::vec3 corners[9];
        GetCornerPoints(corners);
        corners[8] = Center();

        glm::vec3 boxSize = Size();
        float size = glm::length(boxSize);

        // test box's bounding sphere against all planes - removes many false tests, adds one more check
        for (int p = 0; p < 6; p++) {
            float centDist = glm::dot(planes[p], glm::vec4(corners[8], 1));
            if (centDist < -size / 2) return IT_Outside;
        }

        int totalIn = 0;
        size /= 6.0f;  // reduce size to 1/4 (half of radius) for more precision!! // tweaked to 1/6, more sensible

        // test all 8 corners and 9th center point against the planes
        // if all points are behind 1 specific plane, we are out
        // if we are in with all points, then we are fully in
        for (int p = 0; p < 6; p++) {
            int inCount = 9;
            int ptIn = 1;

            for (int i = 0; i < 9; ++i) {
                // test this point against the planes
                float distance = glm::dot(planes[p], glm::vec4(corners[i], 1));
                if (distance < -size) {
                    ptIn = 0;
                    inCount--;
                }
            }

            // were all the points outside of plane p?
            if (inCount == 0) {
                // assert( completelyIn == false );
                return IT_Outside;
            }

            // check if they were all on the right side of the plane
            totalIn += ptIn;
        }

        if (totalIn == 6) return IT_Inside;

        return IT_Intersect;
    }

    float MinDistanceFromPointSq(const glm::vec3& point) {
        float dist = 0.0f;

        if (point.x < Min.x) {
            float d = point.x - Min.x;
            dist += d * d;
        } else if (point.x > Max.x) {
            float d = point.x - Max.x;
            dist += d * d;
        }

        if (point.y < Min.y) {
            float d = point.y - Min.y;
            dist += d * d;
        } else if (point.y > Max.y) {
            float d = point.y - Max.y;
            dist += d * d;
        }

        if (point.z < Min.z) {
            float d = point.z - Min.z;
            dist += d * d;
        } else if (point.z > Max.z) {
            float d = point.z - Max.z;
            dist += d * d;
        }

        return dist;
    }

    float MaxDistanceFromPointSq(const glm::vec3& point) {
        float dist = 0.0f;
        float k;

        k = (std::max)(fabsf(point.x - Min.x), fabsf(point.x - Max.x));
        dist += k * k;

        k = (std::max)(fabsf(point.y - Min.y), fabsf(point.y - Max.y));
        dist += k * k;

        k = (std::max)(fabsf(point.z - Min.z), fabsf(point.z - Max.z));
        dist += k * k;

        return dist;
    }

    bool IntersectSphereSq(const glm::vec3& center, float radiusSq) { return MinDistanceFromPointSq(center) <= radiusSq; }

    bool IsInsideSphereSq(const glm::vec3& center, float radiusSq) { return MaxDistanceFromPointSq(center) <= radiusSq; }

    bool IntersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float& distance) {
        float tmin = -FLT_MAX;  // set to -FLT_MAX to get first hit on line
        float tmax = FLT_MAX;   // set to max distance ray can travel

        float _rayOrigin[] = {rayOrigin.x, rayOrigin.y, rayOrigin.z};
        float _rayDirection[] = {rayDirection.x, rayDirection.y, rayDirection.z};
        float _min[] = {Min.x, Min.y, Min.z};
        float _max[] = {Max.x, Max.y, Max.z};

        const float EPSILON = 1e-5f;

        for (int i = 0; i < 3; i++) {
            if (fabsf(_rayDirection[i]) < EPSILON) {
                // Parallel to the plane
                if (_rayOrigin[i] < _min[i] || _rayOrigin[i] > _max[i]) {
                    // assert( !k );
                    // hits1_false++;
                    return false;
                }
            } else {
                float ood = 1.0f / _rayDirection[i];
                float t1 = (_min[i] - _rayOrigin[i]) * ood;
                float t2 = (_max[i] - _rayOrigin[i]) * ood;

                if (t1 > t2) std::swap(t1, t2);

                if (t1 > tmin) tmin = t1;
                if (t2 < tmax) tmax = t2;

                if (tmin > tmax) {
                    // assert( !k );
                    // hits1_false++;
                    return false;
                }
            }
        }

        distance = tmin;

        return true;
    }

    static AABB Enclosing(const AABB& a, const AABB& b) {
        glm::vec3 bmin, bmax;

        bmin.x = (std::min)(a.Min.x, b.Min.x);
        bmin.y = (std::min)(a.Min.y, b.Min.y);
        bmin.z = (std::min)(a.Min.z, b.Min.z);

        bmax.x = (std::max)(a.Max.x, b.Max.x);
        bmax.y = (std::max)(a.Max.y, b.Max.y);
        bmax.z = (std::max)(a.Max.z, b.Max.z);

        return AABB(bmin, bmax);
    }

    bool operator==(const AABB& b) { return Min == b.Min && Max == b.Max; }

    float BoundingSphereRadius() {
        glm::vec3 size = Size();
        return glm::length(size) * 0.5f;
    }

    void Expand(float percentage) {
        glm::vec3 offset = Size() * percentage;
        Min -= offset;
        Max += offset;
    }
};

#endif  // !MINI_MATH_H
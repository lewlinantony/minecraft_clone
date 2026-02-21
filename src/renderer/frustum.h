#pragma once
#include <glm/glm.hpp>
#include <array>

struct Plane {
    glm::vec3 normal;
    float distance;

    // Normalizing is critical to get correct distances!
    void normalize() {
        float length = glm::length(normal);
        normal /= length;
        distance /= length;
    }
    
    // Returns true if the point is BEHIND the plane (outside the view)
    bool isPointOutside(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance < 0;
    }
};


/*
The View Matrix physically moves the world relative to the camera's eyes
The Projection Matrix scales objects by their depth (W) to create perspective.
Hence View projection Matrix
(think of it as like a multiplier that brings anything in its (range or depth) w in the cameras vicinity)

The GPU divides everything by W (Perspective Divide). This traps the entire visible universe inside the bounds: 
        -W <= (X, Y, Z) <= W

for the X axis: 

        Left bound:  X >= -W  --->  X + W >= 0
        Right bound: X <=  W  --->  W - X >= 0

        Same applies for Y and Z

to get the actual X, Y, Z boundaries practically, we have to reverse engineer the View-Projection matrix (M) 
and multiply it out. For the left plane:

    x(M03 + M00) + y(M13 + M10) + z(M23 + M20) + (M33 + M30) >= 0
    Notice that x(A) + y(B) + z(C) is literally just the Dot Product

where column 0 is X, and column 3 is W

so we take this concept, and try if the world coord of a point(usually its max and min) satisfyies this equation.
so we take the dot product, add/sub the depth distance (+W or -W), and if it's >= 0, it satisfies the equation. 
its inside the frustum. else, cull it.
 */
class Frustum {
    std::array<Plane, 6> planes; 

public:
    // Extracts the 6 planes from the View-Projection Matrix
    void update(const glm::mat4& mat) {
        // Left
        planes[0].normal.x = mat[0][3] + mat[0][0];
        planes[0].normal.y = mat[1][3] + mat[1][0];
        planes[0].normal.z = mat[2][3] + mat[2][0];
        planes[0].distance = mat[3][3] + mat[3][0];

        // Right
        planes[1].normal.x = mat[0][3] - mat[0][0];
        planes[1].normal.y = mat[1][3] - mat[1][0];
        planes[1].normal.z = mat[2][3] - mat[2][0];
        planes[1].distance = mat[3][3] - mat[3][0];

        // Bottom
        planes[2].normal.x = mat[0][3] + mat[0][1];
        planes[2].normal.y = mat[1][3] + mat[1][1];
        planes[2].normal.z = mat[2][3] + mat[2][1];
        planes[2].distance = mat[3][3] + mat[3][1];

        // Top
        planes[3].normal.x = mat[0][3] - mat[0][1];
        planes[3].normal.y = mat[1][3] - mat[1][1];
        planes[3].normal.z = mat[2][3] - mat[2][1];
        planes[3].distance = mat[3][3] - mat[3][1];

        // Near
        planes[4].normal.x = mat[0][3] + mat[0][2];
        planes[4].normal.y = mat[1][3] + mat[1][2];
        planes[4].normal.z = mat[2][3] + mat[2][2];
        planes[4].distance = mat[3][3] + mat[3][2];

        // Far
        planes[5].normal.x = mat[0][3] - mat[0][2];
        planes[5].normal.y = mat[1][3] - mat[1][2];
        planes[5].normal.z = mat[2][3] - mat[2][2];
        planes[5].distance = mat[3][3] - mat[3][2];

        for (auto& plane : planes) plane.normalize();
    }

    // CHECKER
    // Returns true if the box is even partially visible
    bool isBoxVisible(const glm::vec3& min, const glm::vec3& max) const {
        for (const auto& plane : planes) {
            // Find the point on the AABB that is furthest in the direction of the normal (P-Vertex)
            glm::vec3 p(
                plane.normal.x > 0 ? max.x : min.x,
                plane.normal.y > 0 ? max.y : min.y,
                plane.normal.z > 0 ? max.z : min.z
            );

            // If the P-Vertex is behind the plane, then all 8 corners are behind the plane.
            // This allows us to cull using just 1 dot product per plane instead of 8.
            if (plane.isPointOutside(p)) {
                return false;
            }
        }
        return true; 
    }
};
#pragma once


// Hash function for glm::ivec3
namespace std {
    template<>
    struct hash<glm::ivec3> {
        std::size_t operator()(const glm::ivec3& v) const noexcept {
            // Magic primes for spatial hashing
            std::size_t hx = std::hash<int>()(v.x) * 73856093;
            std::size_t hy = std::hash<int>()(v.y) * 19349663;
            std::size_t hz = std::hash<int>()(v.z) * 83492791;
            return hx ^ hy ^ hz;
        }
    };
}
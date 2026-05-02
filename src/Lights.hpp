#pragma once

#include <array>
#include <cstddef>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// ============================================================
// Light source descriptions.
//
// All positions/directions are expressed in WORLD SPACE on the
// CPU side.  set_lighting_uniforms() transforms them to view
// space before uploading to the GPU.
// ============================================================

constexpr std::size_t NUM_POINT_LIGHTS = 3;

// Infinite directional light (sun, moon, etc.)
struct DirectionalLight {
    glm::vec3 direction{-0.3f, -1.0f, -0.5f};  // unit vector, world space
    glm::vec3 ambient  { 0.04f};
    glm::vec3 diffuse  { 1.0f, 0.9f, 0.7f};
    glm::vec3 specular { 1.0f, 0.95f, 0.8f};
};

// Attenuated omnidirectional point lights in SoA form.
struct PointLights {
    std::array<glm::vec3, NUM_POINT_LIGHTS> position{};
    std::array<glm::vec3, NUM_POINT_LIGHTS> ambient{};
    std::array<glm::vec3, NUM_POINT_LIGHTS> diffuse{};
    std::array<glm::vec3, NUM_POINT_LIGHTS> specular{};

    // Quadratic attenuation: 1 / (c + l*d + q*d²)
    std::array<float, NUM_POINT_LIGHTS> constant{};
    std::array<float, NUM_POINT_LIGHTS> linear{};
    std::array<float, NUM_POINT_LIGHTS> quadratic{};

    // Orbit parameters — updated each frame from App::run()
    std::array<glm::vec3, NUM_POINT_LIGHTS> orbitCenter{};
    std::array<float, NUM_POINT_LIGHTS> orbitRadius{};
    std::array<float, NUM_POINT_LIGHTS> orbitHeight{};
    std::array<float, NUM_POINT_LIGHTS> orbitSpeed{};
    std::array<float, NUM_POINT_LIGHTS> orbitAngle{};  // radians, wraps naturally

    void updatePositions() {
        for (std::size_t i = 0; i < NUM_POINT_LIGHTS; ++i) {
            position[i] = orbitCenter[i] + glm::vec3{
                orbitRadius[i] * glm::cos(orbitAngle[i]),
                orbitHeight[i],
                orbitRadius[i] * glm::sin(orbitAngle[i]),
            };
        }
    }
};

// Cone spotlight.  When attached to the camera the position is the
// view-space origin and direction is (0,0,-1) — no transform needed.
struct SpotLight {
    glm::vec3 direction  { 0.0f, 0.0f, -1.0f};  // view space
    glm::vec3 ambient    { 0.0f};
    glm::vec3 diffuse    { 1.0f, 1.0f, 0.9f};
    glm::vec3 specular   { 1.0f, 1.0f, 0.9f};
    float cutoff     {0.9763f};  // cos(12.5°) — inner cone edge
    float outerCutoff{0.9537f};  // cos(17.5°) — outer (soft) edge
    float constant {1.0f};
    float linear   {0.045f};
    float quadratic{0.0075f};
    bool  on{true};
};

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// ============================================================
// Light source descriptions.
//
// All positions/directions are expressed in WORLD SPACE on the
// CPU side.  set_lighting_uniforms() transforms them to view
// space before uploading to the GPU.
// ============================================================

// Infinite directional light (sun, moon, etc.)
struct DirectionalLight {
    glm::vec3 direction{-0.3f, -1.0f, -0.5f};  // unit vector, world space
    glm::vec3 ambient  { 0.04f};
    glm::vec3 diffuse  { 1.0f, 0.9f, 0.7f};
    glm::vec3 specular { 1.0f, 0.95f, 0.8f};
};

// Attenuated omnidirectional point light with an optional orbit animation.
struct PointLight {
    glm::vec3 diffuse;   // must be provided (no sensible default color)
    glm::vec3 specular;
    glm::vec3 ambient  {0.02f};

    // Quadratic attenuation: 1 / (c + l*d + q*d²)
    float constant {1.0f};
    float linear   {0.09f};
    float quadratic{0.032f};

    // Orbit parameters — updated each frame from App::run()
    glm::vec3 orbitCenter{0.0f};  // world-space centre of the orbit
    float orbitRadius{3.0f};
    float orbitHeight{0.0f};
    float orbitSpeed {1.0f};
    float orbitAngle {0.0f};  // radians, wraps naturally

    // Returns the current world-space position based on the orbit state.
    [[nodiscard]] glm::vec3 worldPosition() const {
        return orbitCenter + glm::vec3{
            orbitRadius * glm::cos(orbitAngle),
            orbitHeight,
            orbitRadius * glm::sin(orbitAngle),
        };
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

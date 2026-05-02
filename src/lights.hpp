#pragma once

#include <glm/glm.hpp>
#include <array>

struct MaterialProperties {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;

    MaterialProperties() : ambient(1.0f), diffuse(1.0f), specular(1.0f), shininess(32.0f) {}
    MaterialProperties(glm::vec3 a, glm::vec3 d, glm::vec3 s, float sh)
        : ambient(a), diffuse(d), specular(s), shininess(sh) {}
};

struct LightIntensity {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    LightIntensity() : ambient(0.1f), diffuse(0.9f), specular(0.6f) {}
    LightIntensity(glm::vec3 a, glm::vec3 d, glm::vec3 s)
        : ambient(a), diffuse(d), specular(s) {}
};

struct PointLight {
    glm::vec3 position;   // World space position
    glm::vec3 color;
    float intensity;

    PointLight() : position(0.0f), color(1.0f), intensity(1.0f) {}
    PointLight(glm::vec3 pos, glm::vec3 col, float intens = 1.0f)
        : position(pos), color(col), intensity(intens) {}
};

struct DirectionalLight {
    glm::vec3 direction;  // World space direction
    glm::vec3 color;
    float intensity;

    DirectionalLight() : direction(0.0f, -1.0f, 0.0f), color(1.0f), intensity(1.0f) {}
    DirectionalLight(glm::vec3 dir, glm::vec3 col, float intens = 1.0f)
        : direction(glm::normalize(dir)), color(col), intensity(intens) {}
};

struct LightingSetup {
    bool use_directional_light;
    DirectionalLight directional_light;
    
    bool use_point_lights;
    std::array<PointLight, 3> point_lights;
    int num_point_lights;
    
    LightIntensity light_intensity;
    MaterialProperties material;

    LightingSetup()
        : use_directional_light(false),
          directional_light(),
          use_point_lights(true),
          num_point_lights(0),
          light_intensity(),
          material() {}
};

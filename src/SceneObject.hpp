#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "Model.hpp"

// ============================================================
// Phong material description.
//
// Values are multiplied component-wise with the corresponding
// light intensity in the shader:
//   ambient  = mat.ambient  * light.ambient  * texColor
//   diffuse  = mat.diffuse  * light.diffuse  * texColor * max(N·L, 0)
//   specular = mat.specular * light.specular * pow(max(R·V, 0), shininess)
// ============================================================
struct Material {
    glm::vec3 ambient  {0.2f};
    glm::vec3 diffuse  {0.8f};
    glm::vec3 specular {0.5f};
    float     shininess{32.0f};
};

// ============================================================
// A transparent scene object: model + its material bundled
// together so they can be sorted and rendered as a pair.
//
// The per-object alpha is stored on Model::alpha.
// Set model->is_transparent = true to include it in the
// painter's algorithm pass.
// ============================================================
struct TransparentObject {
    std::shared_ptr<Model> model;
    Material               material;
};

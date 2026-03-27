#pragma once

#include <glm/glm.hpp> 

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;

    bool operator == (const Vertex& v1) const {
        return (Position == v1.Position
            && Normal == v1.Normal
            && TexCoords == v1.TexCoords);
    }
};


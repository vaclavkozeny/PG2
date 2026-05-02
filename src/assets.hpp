#pragma once

#include <GL/glew.h> 
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

//vertex description
struct vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;

    bool operator == (const vertex& v1) const {
        return (position == v1.position
            && normal == v1.normal
            && texCoords == v1.texCoords);
    }
};


#pragma once

#include <GL/glew.h> 
//#include <GL/wglew.h> 
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

//vertex description
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;

    bool operator == (const Vertex& v1) const {
        return (position == v1.position
            && normal == v1.normal
            && tex_coords == v1.tex_coords);
    }
};


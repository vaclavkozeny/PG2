#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp> 
#include <glm/ext.hpp>

#include "assets.hpp"
#include "non_copyable.hpp"

class Mesh: private NonCopyable
{
public:
    // force attribute slots in shaders for all meshes, shaders etc.
    static constexpr GLuint attribute_location_position{0};
    static constexpr GLuint attribute_location_normal{1};
    static constexpr GLuint attribute_location_texture_coords{2};

    // No default constructor. RAII - if constructed, it will be correctly initialized
    // and can be rendered. OpenGL resources are guaranteed to be deallocated using destructor. 
    // Double-free errors are prevented by making class non-copyable (therefore 
    // double destruction of the same OpenGL buffer is prevented). 
    Mesh() = delete; 
    
    // Simple mesh from vertices
    Mesh(std::vector<Vertex> const &vertices, GLenum primitive_type) : primitive_type_{primitive_type}
    {

        
    }
         
    // Mesh with indirect vertex addressing. Needs compiled shader for attributes setup. 
    Mesh(std::vector<Vertex> const &vertices, std::vector<GLuint> const &indices, GLenum primitive_type) :
        Mesh{vertices, primitive_type}
    {
        
    }    

    void draw() {
        glBindVertexArray(vao_);
        
    	if (ebo_ == 0) {
    		glDrawArrays(primitive_type_, 0, vertices.size());
    	} else {
    		glDrawElements(primitive_type_, indices.size(), GL_UNSIGNED_INT, nullptr);
    	}
    }

    ~Mesh() {
    	glDeleteBuffers(1, &ebo_);
    	glDeleteBuffers(1, &vbo_);
    	glDeleteVertexArrays(1, &vao_);
    };
private:
    // safe defaults
    GLenum primitive_type_{GL_POINTS}; 

    // keep the data
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    
    // OpenGL buffer IDs
    // ID = 0 is reserved (i.e. uninitalized)
    GLuint vao_{0};
    GLuint vbo_{0};
    GLuint ebo_{0};
};
  



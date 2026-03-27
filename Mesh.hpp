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

    // Constructor without indices
    Mesh(std::vector<Vertex> const &vertices, GLenum primitive_type) : 
        primitive_type_{primitive_type},
        vertices{vertices},
        indices{}
    {
        setupMesh();
    }
         
    // Constructor with indices (indirect vertex addressing via EBO)
    Mesh(std::vector<Vertex> const &vertices, std::vector<GLuint> const &indices, GLenum primitive_type) :
        primitive_type_{primitive_type},
        vertices{vertices},
        indices{indices}
    {
        setupMesh();
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

    // Setup mesh VAO, VBO, EBO using bind-to-edit (Mac compatible)
    void setupMesh() {
        // Create VAO
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        // Create and fill VBO with vertex data
        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        // Set vertex attributes
        // Position attribute
        glEnableVertexAttribArray(attribute_location_position);
        glVertexAttribPointer(
            attribute_location_position,
            3, GL_FLOAT, GL_FALSE,
            sizeof(Vertex),
            (void*)offsetof(Vertex, position)
        );

        // Normal attribute
        glEnableVertexAttribArray(attribute_location_normal);
        glVertexAttribPointer(
            attribute_location_normal,
            3, GL_FLOAT, GL_FALSE,
            sizeof(Vertex),
            (void*)offsetof(Vertex, normal)
        );

        // Texture coordinates attribute
        glEnableVertexAttribArray(attribute_location_texture_coords);
        glVertexAttribPointer(
            attribute_location_texture_coords,
            2, GL_FLOAT, GL_FALSE,
            sizeof(Vertex),
            (void*)offsetof(Vertex, tex_coords)
        );

        // Setup EBO if indices are provided
        if (!indices.empty()) {
            glGenBuffers(1, &ebo_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        }

        // Unbind VAO (important: do this after EBO binding if used)
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        if (!indices.empty()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
    }
};
  



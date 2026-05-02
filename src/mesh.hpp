#pragma once


#include <GL/glew.h>
#include "assets.hpp"
#include "non_copyable.hpp"

class Mesh: private NonCopyable {
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
    Mesh(std::vector<vertex> const &vertices, GLenum primitive_type) : primitive_type_{primitive_type}
    {
        this->vertices = vertices;
        glCreateVertexArrays(1, &vao_);

        // 2. Vytvoření a naplnění VBO (Buffer pro vrcholy)
        glCreateBuffers(1, &vbo_);
        glNamedBufferData(vbo_, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);

        // 3. Nastavení atributů ve VAO - position, normal, texCoords
        // Position attribute
        glEnableVertexArrayAttrib(vao_, attribute_location_position);
        glVertexArrayAttribFormat(vao_, attribute_location_position, 3, GL_FLOAT, GL_FALSE, offsetof(vertex, position));
        glVertexArrayAttribBinding(vao_, attribute_location_position, 0);

        // Normal attribute
        glEnableVertexArrayAttrib(vao_, attribute_location_normal);
        glVertexArrayAttribFormat(vao_, attribute_location_normal, 3, GL_FLOAT, GL_FALSE, offsetof(vertex, normal));
        glVertexArrayAttribBinding(vao_, attribute_location_normal, 0);

        // Texture coordinates attribute
        glEnableVertexArrayAttrib(vao_, attribute_location_texture_coords);
        glVertexArrayAttribFormat(vao_, attribute_location_texture_coords, 2, GL_FLOAT, GL_FALSE, offsetof(vertex, texCoords));
        glVertexArrayAttribBinding(vao_, attribute_location_texture_coords, 0);

        // 4. Propojení VAO a VBO
        glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, sizeof(vertex));
    }
         
    // Mesh with indirect vertex addressing. Needs compiled shader for attributes setup. 
    Mesh(std::vector<vertex> const &vertices, std::vector<GLuint> const &indices, GLenum primitive_type) :
        Mesh{vertices, primitive_type}
    {
        this->indices = indices;
        primitive_type_ = primitive_type;

        glCreateBuffers(1, &ebo_);
        glNamedBufferData(ebo_, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

        // Propojení EBO s VAO
        glVertexArrayElementBuffer(vao_, ebo_);
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
    std::vector<vertex> vertices;
    std::vector<GLuint> indices;
    
    // OpenGL buffer IDs
    // ID = 0 is reserved (i.e. uninitalized)
    GLuint vao_{0};
    GLuint vbo_{0};
    GLuint ebo_{0};
};




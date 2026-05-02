#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <memory> 
#include <cmath>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include "assets.hpp"
#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include "OBJloader.hpp"
#include "Texture.hpp"

class Model {
public:
    // origin point of whole model
    glm::vec3 pivot_position{}; // [0,0,0] of the object
    glm::vec3 eulerAngles{};    // pitch, yaw, roll (in degrees)
    glm::vec3 scale{1.0f};

    // Transparency (Task 1 — 09cv)
    // Set is_transparent=true and alpha<1.0 to enable the painter's algorithm
    // path and blended rendering for this model.
    bool  is_transparent{false};
    float alpha{1.0f};  // 0 = invisible, 1 = fully opaque

    // mesh related data
    struct mesh_package {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<ShaderProgram> shader;
        std::shared_ptr<Texture> texture{nullptr}; // nullptr = no texture

        glm::vec3 origin;
        glm::vec3 eulerAngles;
        glm::vec3 scale;
    };
    std::vector<mesh_package> meshes;
    
    // Cache model matrix (updated only when needed)
    glm::mat4 local_model_matrix{1.0f};
    
    // Constructors
    Model() = default;
    
    // Load model from OBJ file
    Model(const std::filesystem::path & filename, std::shared_ptr<ShaderProgram> shader) {
        // Load mesh (all meshes) of the model, (in the future: load material of each mesh, load textures...)
        // notice: you can load multiple meshes and place them to proper positions, 
        //            multiple textures (with reusing) etc. to construct single complicated Model   
        //
        // This can be done by extending OBJ file parser (OBJ can load hierarchical models),
        // or by your own JSON model specification (or keep it simple and set a rule: 1model=1mesh ...) 
        //
        
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        
        if (loadOBJ(filename, vertices, indices)) {
            auto mesh = std::make_shared<Mesh>(vertices, indices, GL_TRIANGLES);
            meshes.emplace_back(mesh, shader, nullptr, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
            // Set default scale for the entire model
            scale = glm::vec3(1.0f);
            pivot_position = glm::vec3(0.0f);
            eulerAngles = glm::vec3(0.0f);
            // Update the model matrix
            local_model_matrix = createModelMatrix(pivot_position, eulerAngles, glm::vec3(1.0f));
        } else {
            throw std::runtime_error("Failed to load OBJ file: " + filename.string());
        }
    }

    // === Helper functions ===
    
    // Helper function: wrap angles to [0, 360)
    static float wrapAngle(float angle) {
        angle = std::fmod(angle, 360.0f);
        if (angle < 0.0f) {
            angle += 360.0f;
        }
        return angle;
    }
    
    // Create model matrix from position, euler angles, and scale
    static glm::mat4 createModelMatrix(const glm::vec3 & origin, 
                                        const glm::vec3 & eulerAngles, 
                                        const glm::vec3 & scale) {
        // Keep angles in proper range
        glm::vec3 eA {wrapAngle(eulerAngles.x), wrapAngle(eulerAngles.y), wrapAngle(eulerAngles.z)};
        
        glm::mat4 t = glm::translate(glm::mat4(1.0f), origin);
        
        // Create rotation matrix using euler angles (pitch, yaw, roll)
        // Using ZYX order: first rotation around Z (roll), then Y (yaw), then X (pitch)
        glm::mat4 rotm = glm::eulerAngleXYZ(glm::radians(eA.x), glm::radians(eA.y), glm::radians(eA.z));
        
        glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
        
        return t * rotm * s;
    }

    // === Transformation Methods ===
    
    void setPosition(const glm::vec3 & new_position) {
        pivot_position = new_position;
        local_model_matrix = createModelMatrix(pivot_position, eulerAngles, scale);        
    }
    
    void setEulerAngles(const glm::vec3 & new_eulerAngles) {
        eulerAngles = new_eulerAngles;
        local_model_matrix = createModelMatrix(pivot_position, eulerAngles, scale);        
    }
    
    void setScale(const glm::vec3 & new_scale) {
        scale = new_scale;
        local_model_matrix = createModelMatrix(pivot_position, eulerAngles, scale);        
    }
    
    // For complex (externally provided) transformations 
    void setModelMatrix(const glm::mat4 & modelm) {
        local_model_matrix = modelm;
    }
    
    void translate(const glm::vec3 & offset) {
        pivot_position += offset;        
        local_model_matrix = createModelMatrix(pivot_position, eulerAngles, scale);        
    }
    
    void rotate(const glm::vec3 & pitch_yaw_roll_offs) {
        eulerAngles += pitch_yaw_roll_offs;
        local_model_matrix = createModelMatrix(pivot_position, eulerAngles, scale);        
    }
    
    void scaleUniform(const glm::vec3 & scale_offset) {
        scale *= scale_offset; 
        local_model_matrix = createModelMatrix(pivot_position, eulerAngles, scale);        
    }
    
    // === Getters ===
    
    glm::mat4 getModelMatrix() const {
        return local_model_matrix;
    }
    
    glm::vec3 getPosition() const {
        return pivot_position;
    }
    
    glm::vec3 getEulerAngles() const {
        return eulerAngles;
    }
    
    glm::vec3 getScale() const {
        return scale;
    }

    // === Mesh management ===

    void addMesh(std::shared_ptr<Mesh> mesh,
                 std::shared_ptr<ShaderProgram> shader,
                 std::shared_ptr<Texture> texture = nullptr,
                 glm::vec3 origin = glm::vec3(0.0f),
                 glm::vec3 eulerAngles = glm::vec3(0.0f),
                 glm::vec3 scale = glm::vec3(1.0f)) {
        meshes.emplace_back(mesh, shader, texture, origin, eulerAngles, scale);
    }

    // update based on running time
    void update(const float delta_t) {
        // change internal state of the model (positions of meshes, size, etc.) 
        // note: this allows dynamic behaviour - it can be modified to 
        //       use lambda funtion, call scripting language, etc. 
    }
    
};


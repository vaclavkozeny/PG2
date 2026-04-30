#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <memory> 

#include <GL/glew.h>
#include <glm/glm.hpp> 
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include <iostream>
#include <stdexcept>

#include "assets.hpp"
#include "mesh.hpp"
#include "shader_program.hpp"
#include "OBJloader.hpp"
#include "texture.hpp"

class Model {
private:
    // origin point of whole model
    glm::vec3 pivot_position{}; // [0,0,0] of the object
    glm::vec3 eulerAngles{};    // pitch, yaw, roll
    glm::vec3 scale{1.0f};
    glm::mat4 local_model_matrix{1.0};
    // mesh related data
    struct mesh_package {
        std::shared_ptr<Mesh> mesh;         // geometry & topology, vertex attributes
        std::shared_ptr<shader_program> shader;     // which shader to use to draw this part of the model
        std::shared_ptr<Texture> texture;

        glm::vec3 origin;                   // mesh origin relative to origin of the whole model
        glm::vec3 eulerAngles;              // mesh rotation relative to orientation of the whole model
        glm::vec3 scale;                    // mesh scale relative to scale of the whole model
    };
    std::vector<mesh_package> meshes;
    
    glm::mat4 createMM(const glm::vec3 & origin, const glm::vec3 & eAng, const glm::vec3 & scale) {
        // keep angles in proper range
        glm::vec3 eA {wrapAngle(eAng.x), wrapAngle(eAng.y), wrapAngle(eAng.z)};
        
    	glm::mat4 t = glm::translate(glm::mat4(1.0f), origin);
		glm::mat4 rotm = glm::yawPitchRoll(glm::radians(eA.y), glm::radians(eA.x), glm::radians(eA.z)); //yaw, pitch, roll
		glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);

        return t * rotm * s;
    }
    float wrapAngle(float angle) { // wrap any float to [0, 360)
        angle = std::fmod(angle, 360.0f);
        if (angle < 0.0f) {
            angle += 360.0f;
        }
        return angle;
    }    
public:
    Model() = default;
    Model(const std::filesystem::path & filename, std::shared_ptr<shader_program> shader, std::shared_ptr<Texture> text) {
        std::vector<vertex> vertices;
        std::vector<GLuint> indices;

        // 1. Call your custom OBJ loader
        bool success = loadOBJ(filename, vertices, indices);

        if (!success) {
            std::cerr << "Model Constructor Error: Failed to load " << filename << std::endl;
            // Depending on your engine's design, you might want to throw an exception here
            // throw std::runtime_error("Failed to load model");
            return;
        }
        auto loaded_mesh = std::make_shared<Mesh>(vertices, indices,GL_TRIANGLES);

        // 2. Construct the single mesh and add it to the model's mesh list
        // This assumes your Mesh class takes vertices, indices, and a shader
        meshes.push_back({
            loaded_mesh,               // mesh
            shader,
            text,                        // texture
            glm::vec3(0.0f),           // origin
            glm::vec3(0.0f),           // eulerAngles
            glm::vec3(1.0f)            // scale
        });
    }

    void addMesh(std::shared_ptr<Mesh> mesh,
                 std::shared_ptr<shader_program> shader,
                 std::shared_ptr<Texture> text, // default value = no texture
                 glm::vec3 origin = glm::vec3(0.0f),      // dafault value
                 glm::vec3 eulerAngles = glm::vec3(0.0f), // dafault value
                 glm::vec3 scale = glm::vec3(1.0f)       // dafault value
                 ) {
        meshes.emplace_back(mesh,shader,text,origin,eulerAngles,scale);
    }

    // update based on running time
    void update(const float delta_t) {
        // change internal state of the model (positions of meshes, size, etc.) 
        // note: this allows dynamic behaviour - it can be modified to 
        //       use lambda funtion, call scripting language, etc. 
    }
    
    void draw() {
        // call draw() on mesh (all meshes)
        for (auto const& mesh_pkg : meshes) {
            mesh_pkg.shader->use(); // select proper shader
            // Set the model matrix for this mesh
            glm::mat4 mesh_model_matrix = createMM(mesh_pkg.origin, mesh_pkg.eulerAngles, mesh_pkg.scale);
            mesh_pkg.shader->setUniform("uM_m", local_model_matrix * mesh_model_matrix);
            
            mesh_pkg.texture->bind();
            mesh_pkg.shader->setUniform("tex0", 0);
            mesh_pkg.mesh->draw();   // draw mesh at proper position
        }
    }
    // ### NEW  (similar can be created for Mesh class)
    void setPosition(const glm::vec3 & new_position) {
        pivot_position = new_position;
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);        
    }
    
    void setEulerAngles(const glm::vec3 & new_eulerAngles) {
        eulerAngles = new_eulerAngles;
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);        
    }
    
    void setScale(const glm::vec3 & new_scale) {
        scale = new_scale;
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);        
    }
    
    // for complex (externally provided) transformations 
    void setModelMatrix(const glm::mat4 & modelm) {
        local_model_matrix = modelm;
    }
    
    void translate(const glm::vec3 & offset) {
        pivot_position += offset;        
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);        
    }
    
    void rotate(const glm::vec3 & pitch_yaw_roll_offs) {
        eulerAngles += pitch_yaw_roll_offs;
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);        
    }
    
    void multiplyScale(const glm::vec3 & scale_offs) {
        scale *= scale_offs; 
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);        
    }
    bool is_transparent {false};   

    glm::vec3 getPosition() { 
        // get 3 values from last column of cached model matrix = translation
        return glm::vec3(local_model_matrix[3]);
    }    
};


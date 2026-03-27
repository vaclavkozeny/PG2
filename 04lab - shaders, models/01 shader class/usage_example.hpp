#include <unordered_map>
#include <memory>  // smart pointers

#include "ShaderProgram.hpp"

class App { 
//...

private:

    // shared library of shaders for all models, automatic resource management 
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> shader_library;
    
};

App::init_assets() {
    // load all shaders to be used in the program 
    shader_library.emplace("simple_shader", std::make_shared<ShaderProgram>("path_to.vert","path_to.frag"));
    shader_library.emplace("rainbow", std::make_shared<ShaderProgram>("path_to.vert","rainbow.frag"));
    
}

App::run() {


    while(...) {

        auto current_shader = shader_library.at("simple_shader"); // crated a copy of shared pointer. Shader is guaranteed to live.
        
        current_shader->use();
        current_shader->setUniform("color", glm::vec3(r,g,b));
        
        //... draw using simple_shader
         
        
        current_shader = shader_library.at("rainbow");
        current_shader->use();
        current_shader->setUniform("iTime", static_cast<float>(glfwGetTime()));
        
        // draw with animated rainbow...
    
    }

}


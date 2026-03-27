#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>

#include <GL/glew.h> 
#include <glm/glm.hpp>

#include "non_copyable.hpp"

class ShaderProgram : private NonCopyable {
public:
    // No default constructor. RAII - if constructed, it will be correctly initialized
    // and can be rendered. OpenGL resources are guaranteed to be deallocated using destructor. 
    // Double-free errors are prevented by making class non-copyable (therefore 
    // double destruction of the same OpenGL shader is prevented). 

    GLuint getID(void) { return ID; }
    GLint  getAttribLocation(const std::string & name);
    
    // set uniform according to name 
    // https://docs.gl/gl4/glUniform
    void setUniform(const std::string & name, const GLfloat val);      
    void setUniform(const std::string & name, const GLint val);        
    void setUniform(const std::string & name, const glm::vec3 & val);  
    void setUniform(const std::string & name, const glm::vec4 & val);  
    void setUniform(const std::string & name, const glm::mat3 & val);   
    void setUniform(const std::string & name, const glm::mat4 & val);
    void setUniform(const std::string & name, const std::vector<GLint> & val);
    void setUniform(const std::string & name, const std::vector<GLfloat> & val);
    void setUniform(const std::string & name, const std::vector<glm::vec3> & val);

    ShaderProgram() = delete;
    explicit ShaderProgram(const std::string& vertex_shader_code, const std::string& fragment_shader_code);
    explicit ShaderProgram(const std::filesystem::path& VS_file, const std::filesystem::path& FS_file);
    ~ShaderProgram();

    void use();
    void deactivate();
    
    
private:
    GLuint ID{0};
    inline static GLuint currently_used_ID{0};
    std::unordered_map<std::string, GLuint> uniform_location_cache;

    GLuint getUniformLocation(const std::string & name);

    std::string textFileRead(const std::filesystem::path & filename); // load text file

    GLuint compile_shader(const std::string & source_code, const GLenum type); 
    std::string getShaderInfoLog(const GLuint obj);    

    GLuint link_shader(const std::vector<GLuint> shader_ids); 
    std::string getProgramInfoLog(const GLuint obj);      
};


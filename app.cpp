

#include <iostream>
#include <format>
#include <chrono>
#include <stack>
#include <random>

#include <opencv2/opencv.hpp>
#include <GL/glew.h> 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "app.hpp"
#include "assets.hpp"
#include "getinfo.hpp"
#include "gl_err_callback.h"

void error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}
App::App()
{
    // default constructor
    // nothing to do here (so far...)
    std::cout << "Constructed...\n";
}
bool App::init()
{
    try {
        // GL init
    {
    	// init glfw
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
            throw std::runtime_error("GLWF not ok!");
    	// open window (GL canvas) with no special properties
    	window = glfwCreateWindow(800, 600, "OpenGL context", NULL, NULL);
        if (!window)
            throw std::runtime_error("GLFW window init failed");
        glfwMakeContextCurrent(window);
        
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, key_callback);
        glfwSwapInterval(0);
       	// init glew
    	auto glew = glewInit();
        if (glew != GLEW_OK)
            throw std::runtime_error(std::format("GLEW not ok! Error: {0}",reinterpret_cast<const char*>(glewGetErrorString(glew))));

        if (!GLEW_ARB_direct_state_access)
            throw std::runtime_error("No DSA :-(");
        if (GLEW_ARB_debug_output)
        {
            glDebugMessageCallback(MessageCallback, 0);
            glEnable(GL_DEBUG_OUTPUT);
            
            //default is asynchronous debug output, use this to simulate glGetError() functionality
            //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            
            std::cout << "GL_DEBUG enabled." << std::endl;
        }
        else
            std::cout << "GL_DEBUG NOT SUPPORTED!" << std::endl;
            
        auto h = getInfo();
        std::cout << h.vendor << "\n" << h.renderer << "\n" << h.version << "\n" << h.glsl << std::endl;
        std::cout << getProfile() << std::endl;   
    }
        
    init_assets();
    }
    catch (std::exception const& e) {
        std::cerr << "Init failed : " << e.what() << std::endl;
        throw;
    }
    std::cout << "Initialized...\n";
    return true;
}
void App::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    App* app = (App*)(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_V && action == GLFW_PRESS) {
        app->vsync_enabled = !app->vsync_enabled;
        glfwSwapInterval(app->vsync_enabled ? 1 : 0);
        //std::cout << "VSync " << (app->vsync_enabled ? "ON" : "OFF") << std::endl;
    }
}
void App::init_assets(void) {
    //
    // Initialize pipeline: compile, link and use shaders
    //
    
    //SHADERS - define & compile & link
    const char* vertex_shader =
        "#version 460 core\n"
        "in vec3 attribute_Position;"
        "void main() {"
        "  gl_Position = vec4(attribute_Position, 1.0);"
        "}";

    const char* fragment_shader =
        "#version 460 core\n"
        "uniform vec4 uniform_Color;"
        "out vec4 FragColor;"
        "void main() {"
        "  FragColor = uniform_Color;"
        "}";
    
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);
    
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);
    
    shader_prog_ID = glCreateProgram();
    glAttachShader(shader_prog_ID, fs);
    glAttachShader(shader_prog_ID, vs);
    glLinkProgram(shader_prog_ID);
    
    //now we can delete shader parts (they can be reused, if you have more shaders)
    //the final shader program already linked and stored separately
    glDetachShader(shader_prog_ID, fs);
    glDetachShader(shader_prog_ID, vs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // 
    // Create and load data into GPU using OpenGL DSA (Direct State Access)
    //
    
    // Create VAO + data description (similar to container)
    glCreateVertexArrays(1, &VAO_ID);

    GLint position_attrib_location = glGetAttribLocation(shader_prog_ID, "attribute_Position");

    glEnableVertexArrayAttrib(VAO_ID, position_attrib_location);
    glVertexArrayAttribFormat(VAO_ID, position_attrib_location, glm::vec3::length(), GL_FLOAT, GL_FALSE, offsetof(vertex, position));
    glVertexArrayAttribBinding(VAO_ID, position_attrib_location, 0); // (GLuint vaobj, GLuint attribindex, GLuint bindingindex)

    // Create and fill data
    glCreateBuffers(1, &VBO_ID);
    glNamedBufferData(VBO_ID, triangle_vertices.size()*sizeof(vertex), triangle_vertices.data(), GL_STATIC_DRAW);

    // Connect together
    glVertexArrayVertexBuffer(VAO_ID, 0, VBO_ID, 0, sizeof(vertex)); // (GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)
}
int App::run(void)
{
    GLfloat r,g,b,a;
    r=g=b=a=1.0f; //white color

    // Activate shader program. There is only one program, so activation can be out of the loop. 
    // In more realistic scenarios, you will activate different shaders for different 3D objects.
    glUseProgram(shader_prog_ID);

    // Get uniform location in GPU program. This will not change, so it can be moved out of the game loop.
    GLint uniform_color_location = glGetUniformLocation(shader_prog_ID, "uniform_Color");
    if (uniform_color_location == -1) {
        std::cerr << "Uniform location is not found in active shader program. Did you forget to activate it?\n";
    }
    
    double lastTime = glfwGetTime();
    int nbFrames = 0;
    try {
        while (!glfwWindowShouldClose(window)) {
            double currentTime = glfwGetTime();
            nbFrames++;
            if (currentTime - lastTime >= 1.0)
            {
                std::string vsync_str = vsync_enabled ? "ON" : "OFF";
                std::string title = "FPS: " + std::to_string(nbFrames) + " | VSync: " + vsync_str;
                glfwSetWindowTitle(window, title.c_str());
                nbFrames = 0;
                lastTime += 1.0;
            }
            // clear canvas
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //set uniform parameter for shader
            // (try to change the color in key callback)          
            glUniform4f(uniform_color_location, r, g, b, a);
            
            //bind 3d object data
            glBindVertexArray(VAO_ID);

            // draw all VAO data
            glDrawArrays(GL_TRIANGLES, 0, triangle_vertices.size());

            // poll events, call callbacks, flip back<->front buffer
            glfwPollEvents();
            glfwSwapBuffers(window);    
        }
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << "Finished OK...\n";
    return EXIT_SUCCESS;
}

App::~App()
{
    // clean-up
    glDeleteProgram(shader_prog_ID);
    glDeleteBuffers(1, &VBO_ID);
    glDeleteVertexArrays(1, &VAO_ID);
    glfwTerminate();
    cv::destroyAllWindows();
    std::cout << "Bye...\n";
}
// icp.cpp 
// author: JJ
#include "assets.hpp"
#include <GLFW/glfw3.h>

#pragma once

class App {
public:
    App();

    bool init(void);
    int run(void);
    void initOpenGL(void);
    void init_assets(void);
    GLFWwindow* window = nullptr;

    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    float triangle_r = 0.0f;

    double lastTime = 0.0;
    int frameCount = 0;

    bool vsync = false;

    ~App();
private:

    GLuint shader_prog_ID{ 0 };
    GLuint VBO_ID{ 0 };
    GLuint VAO_ID{ 0 };
    
    std::vector<vertex> triangle_vertices =
    {
    	{{0.0f,  0.5f,  0.0f}},
    	{{0.5f, -0.5f,  0.0f}},
    	{{-0.5f, -0.5f,  0.0f}}
    };
    
};


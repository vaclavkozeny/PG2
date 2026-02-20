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

    ~App();
private:
    GLFWwindow* window = nullptr;

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


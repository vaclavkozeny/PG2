#pragma once
#include "assets.hpp"
#include <GLFW/glfw3.h>
class App {
public:
    App();
    GLFWwindow * window = nullptr;

    bool init(void);
    int run(void);
    void init_assets(void);

    ~App();
private:
    bool vsync_enabled = false;
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
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

#pragma once
#include "assets.hpp"
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <memory>  // smart pointers

#include "shader_program.hpp"
#include "mesh.hpp"
#include "model.hpp"

class App {
public:
    App();
    GLFWwindow * window = nullptr;

    bool init(void);
    int run(void);
    void init_imgui();
    void destroy(void);
    ~App();
private:
    bool vsync_enabled = false;
    bool show_imgui = true;

    bool isFullScreen = false;
    int savedXPos = 0, savedYPos = 0;
    int savedWidth = 800, savedHeight = 600;

    std::unordered_map<std::string, std::shared_ptr<shader_program>> shader_library;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> mesh_library;
    std::unordered_map<std::string, Model> scene; 

    void init_assets(void);
    void init_gl(void);
    void load_config(std::string filename);
    GLFWmonitor* GetCurrentMonitor(GLFWwindow* window);
    void toggle_fullscreen(GLFWwindow* window);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    
    GLuint shader_prog_ID{ 0 };
    GLuint VBO_ID{ 0 };
    GLuint VAO_ID{ 0 };
    GLfloat triangle_r, triangle_g, triangle_b = 1.0f;
    GLfloat bg_r, bg_g, bg_b = 0.0f;
    std::vector<vertex> triangle_vertices =
    {
        {{-0.5f, -0.5f,  0.0f}},
        {{ 0.5f, -0.5f,  0.0f}},
        {{ 0.5f,  0.5f,  0.0f}},

        {{-0.5f, -0.5f,  0.0f}},
        {{ 0.5f,  0.5f,  0.0f}},
        {{-0.5f,  0.5f,  0.0f}} 
    }; 
    struct WindowConfig {
        int width;
        int height;
        std::string title;
        bool vsync;
    };
    WindowConfig config;
    
};

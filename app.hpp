#pragma once
#include "assets.hpp"
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <memory>  // smart pointers
#include <opencv2/opencv.hpp>

#include "shader_program.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "camera.hpp"
#include "texture.hpp"

class App {
public:
    App();
    GLFWwindow * window = nullptr;

    bool init(void);
    int run(void);
    void init_imgui();
    void destroy(void);
    void update_projection_matrix(void);
    ~App();
private:
    bool vsync_enabled;
    bool msaa_enabled;
    bool show_imgui = true;

    bool isFullScreen = false;
    int savedXPos = 0, savedYPos = 0;
    int savedWidth = 800, savedHeight = 600;

    std::unordered_map<std::string, std::shared_ptr<shader_program>> shader_library;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> mesh_library;
    std::unordered_map<std::string, std::shared_ptr<Texture>> texture_library;
    std::unordered_map<std::string, Model> scene; 

    void init_assets(void);
    void init_gl(void);
    void load_config(std::string filename);
    void save_screenshot(void);
    GLFWmonitor* GetCurrentMonitor(GLFWwindow* window);
    void toggle_fullscreen(GLFWwindow* window);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void fbsize_callback(GLFWwindow* window, int width, int height);
    static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    GLuint shader_prog_ID{ 0 };
    GLuint VBO_ID{ 0 };
    GLuint VAO_ID{ 0 };
    GLfloat triangle_r{1.0f}, triangle_g{1.0f}, triangle_b{1.0f};
    GLfloat bg_r{0.0f}, bg_g{0.0f}, bg_b{0.0f};
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
        bool msaa;
    };
    WindowConfig config;
    // projection related variables    
    int width{0}, height{0};
    float fov{60.0f};
    // store projection matrix here, update only on callbacks
    glm::mat4 projection_matrix{glm::identity<glm::mat4>()};
    // camera related 
    Camera camera;
    // remember last cursor position, move relative to that in the next frame
    double cursorLastX{ 0 };
    double cursorLastY{ 0 };
};

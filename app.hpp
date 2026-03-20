// icp.cpp 
// author: JJ
#include "assets.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ShaderProgram.hpp"
#include "Model.hpp"
#include "camera.hpp"

#pragma once

class App {
public:
    App();

    bool init(void);
    int run(void);
    void initOpenGL(void);
    void init_imgui();
    void init_assets(void);
    GLFWwindow* window = nullptr;

    bool isFullScreen = false;
    int savedXPos = 0, savedYPos = 0;
    int savedWidth = 800, savedHeight = 600;

    static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void toggle_fullscreen(GLFWwindow* window);
    GLFWmonitor* GetCurrentMonitor(GLFWwindow* window);

    float triangle_r = 0.0f;

    double lastTime = 0.0;
    int frameCount = 0;

    bool vsync = true;

    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> shader_library;
    std::shared_ptr<Model> model;

    ~App();
private:

    GLuint shader_prog_ID{ 0 };
    GLuint VBO_ID{ 0 };
    GLuint VAO_ID{ 0 };
    
    // === Transformation related variables ===
    int width{800}, height{600};
    float fov{60.0f};
    glm::mat4 projection_matrix{glm::identity<glm::mat4>()};
    
    // Camera
    Camera camera{glm::vec3(0.0f, 0.0f, 2.0f)};
    double cursorLastX{0.0};
    double cursorLastY{0.0};
    double last_frame_time{0.0};
    
    std::vector<Vertex> triangle_vertices =
    {
    	{{0.0f,  0.5f,  0.0f}, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f)},
    	{{0.5f, -0.5f,  0.0f}, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f)},
    	{{-0.5f, -0.5f,  0.0f}, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f)}
    };
    
    // should be ImGUI window displayed?
    bool show_imgui{true};

    void init_glfw(void);
    
    // === Transformation related methods ===
    void update_projection_matrix(void);

    //callbacks
    static void glfw_error_callback(int error, const char* description);
    static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
};


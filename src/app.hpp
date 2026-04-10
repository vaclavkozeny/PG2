#pragma once

#include <array>
#include <unordered_map>
#include <memory>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "assets.hpp"
#include "ShaderProgram.hpp"
#include "Model.hpp"
#include "Texture.hpp"
#include "Camera.hpp"

// ============================================================
// Light data structures
// ============================================================

struct DirectionalLight {
    glm::vec3 direction{-0.3f, -1.0f, -0.5f}; // world space
    glm::vec3 ambient  {0.04f, 0.04f, 0.04f};
    glm::vec3 diffuse  {1.0f,  0.9f,  0.7f};
    glm::vec3 specular {1.0f,  0.95f, 0.8f};
};

struct PointLight {
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 ambient  {0.02f, 0.02f, 0.02f};
    float constant {1.0f};
    float linear   {0.09f};
    float quadratic{0.032f};
    // Orbit parameters (animation)
    float orbitRadius{3.0f};
    float orbitHeight{0.0f};
    float orbitSpeed {1.0f};
    float orbitAngle {0.0f};   // updated each frame

    glm::vec3 currentWorldPos() const {
        return glm::vec3(
            orbitRadius * glm::cos(orbitAngle),
            orbitHeight,
            orbitRadius * glm::sin(orbitAngle)
        );
    }
};

struct SpotLight {
    glm::vec3 direction  {0.0f,  0.0f, -1.0f}; // view space: camera looks down -Z
    glm::vec3 ambient    {0.0f,  0.0f,  0.0f};
    glm::vec3 diffuse    {1.0f,  1.0f,  0.9f};
    glm::vec3 specular   {1.0f,  1.0f,  0.9f};
    float cutoff     {0.9763f};  // cos(12.5 deg)
    float outerCutoff{0.9537f};  // cos(17.5 deg)
    float constant {1.0f};
    float linear   {0.045f};
    float quadratic{0.0075f};
    bool  on{true};
};

// ============================================================

class App {
public:
    App();
    ~App();

    bool init();
    int  run();

    // GLFW window
    GLFWwindow* window{nullptr};

    // Fullscreen state
    bool isFullScreen{false};
    int  savedXPos{0}, savedYPos{0};
    int  savedWidth{800}, savedHeight{600};

    // UI toggles (used by key callbacks)
    bool show_imgui{true};

    // VSync
    bool vsync{true};

    // Asset libraries
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> shader_library;
    std::unordered_map<std::string, std::shared_ptr<Texture>>       texture_library;
    std::shared_ptr<Model> model;
    std::shared_ptr<Model> textured_model;

    // ---- Lighting ----
    DirectionalLight              dirLight;
    std::array<PointLight, 3>     pointLights;
    SpotLight                     spotLight;

    // Windowing helpers
    void         toggle_fullscreen(GLFWwindow* window);
    GLFWmonitor* GetCurrentMonitor(GLFWwindow* window);

    // GLFW callbacks (static — retrieved via glfwGetWindowUserPointer)
    static void glfw_error_callback(int error, const char* description);
    static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

private:
    // Viewport / projection
    int   width{800}, height{600};
    float fov{60.0f};
    glm::mat4 projection_matrix{glm::identity<glm::mat4>()};

    // Camera
    Camera camera{glm::vec3(0.0f, 0.5f, 5.0f)};
    double cursorLastX{0.0};
    double cursorLastY{0.0};
    double last_frame_time{0.0};

    // FPS counter
    double lastTime{0.0};
    int    frameCount{0};

    // Initialization helpers
    void initOpenGL();
    void init_imgui();
    void init_assets();
    void update_projection_matrix();

    // Lighting helpers
    void set_lighting_uniforms(const std::shared_ptr<ShaderProgram>& shader,
                               const glm::mat4& view_matrix) const;
};

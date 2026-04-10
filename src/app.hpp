#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "assets.hpp"
#include "Camera.hpp"
#include "Model.hpp"
#include "Particle.hpp"
#include "ShaderProgram.hpp"
#include "Texture.hpp"

// ============================================================
// Lighting (from 08cv — unchanged)
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
    // Orbit animation
    float orbitRadius{3.0f};
    float orbitHeight{0.0f};
    float orbitSpeed {1.0f};
    float orbitAngle {0.0f};

    glm::vec3 currentWorldPos() const {
        return glm::vec3(
            orbitRadius * glm::cos(orbitAngle),
            orbitHeight,
            orbitRadius * glm::sin(orbitAngle)
        );
    }
};

struct SpotLight {
    glm::vec3 direction  {0.0f,  0.0f, -1.0f};
    glm::vec3 ambient    {0.0f,  0.0f,  0.0f};
    glm::vec3 diffuse    {1.0f,  1.0f,  0.9f};
    glm::vec3 specular   {1.0f,  1.0f,  0.9f};
    float cutoff     {0.9763f};  // cos(12.5°)
    float outerCutoff{0.9537f};  // cos(17.5°)
    float constant {1.0f};
    float linear   {0.045f};
    float quadratic{0.0075f};
    bool  on{true};
};

// ============================================================
// Material — Phong surface description
// ============================================================

struct Material {
    glm::vec3 ambient {0.2f, 0.2f, 0.2f};
    glm::vec3 diffuse {0.8f, 0.8f, 0.8f};
    glm::vec3 specular{0.5f, 0.5f, 0.5f};
    float shininess{32.0f};
};

// ============================================================
// GlassObject — transparent model paired with its material
// ============================================================

struct GlassObject {
    std::shared_ptr<Model> model;
    Material               mat;
};

// ============================================================
// EnemyState — simple chasing sphere (Task 2)
// ============================================================

struct EnemyState {
    glm::vec3 position   {-4.0f, 0.75f, -4.0f};
    glm::vec3 velocity   {0.0f};
    float     radius     {0.75f};
    float     hitCooldown{0.0f};
    static constexpr float SPEED = 1.5f;  // world units / second
};

// ============================================================
// Application
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

    // UI / vsync
    bool show_imgui{true};
    bool vsync{true};

    // Asset libraries
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> shader_library;
    std::unordered_map<std::string, std::shared_ptr<Texture>>       texture_library;

    // ---- Scene models ----
    std::shared_ptr<Model> model;           // bunny (opaque)
    std::shared_ptr<Model> textured_model;  // main textured sphere (opaque)
    std::shared_ptr<Model> ground;          // ground plane (opaque)
    std::shared_ptr<Model> enemy_model;     // enemy sphere (opaque, Task 2)

    // Task 1: Transparent glass spheres (painter's algorithm)
    std::vector<GlassObject> transparent_objects;

    // Task 2: Enemy + collision constants
    EnemyState enemy;
    static constexpr float CAMERA_RADIUS = 0.4f;   // approximate player sphere radius
    static constexpr float MAP_HALF_SIZE = 12.0f;   // XZ boundary ±12 units
    static constexpr float FLOOR_Y       = 0.0f;    // ground plane Y

    // Task 3: Particle system (tetrahedron particles)
    ParticleSystem particle_system;

    // ---- Lighting ----
    DirectionalLight          dirLight;
    std::array<PointLight, 3> pointLights;
    SpotLight                 spotLight;

    // Window helpers
    void         toggle_fullscreen(GLFWwindow* win);
    GLFWmonitor* GetCurrentMonitor(GLFWwindow* win);

    // GLFW callbacks
    static void glfw_error_callback(int error, const char* description);
    static void glfw_framebuffer_size_callback(GLFWwindow*, int w, int h);
    static void glfw_key_callback(GLFWwindow*, int key, int scancode, int action, int mods);
    static void glfw_mouse_button_callback(GLFWwindow*, int button, int action, int mods);
    static void glfw_cursor_position_callback(GLFWwindow*, double xpos, double ypos);
    static void glfw_scroll_callback(GLFWwindow*, double xoffset, double yoffset);

private:
    // Viewport / projection
    int   width{800}, height{600};
    float fov{60.0f};
    glm::mat4 projection_matrix{glm::identity<glm::mat4>()};

    // Camera — starts 1.5 m above floor, 8 m back
    Camera camera{glm::vec3(0.0f, 1.5f, 8.0f)};
    double cursorLastX{0.0}, cursorLastY{0.0};
    double last_frame_time{0.0};

    // FPS counter
    double lastTime{0.0};
    int    frameCount{0};

    // Initialization helpers
    void initOpenGL();
    void init_imgui();
    void init_assets();
    void update_projection_matrix();

    // Upload all light uniforms to a shader program
    void set_lighting_uniforms(const std::shared_ptr<ShaderProgram>& shader,
                               const glm::mat4& view_matrix) const;
};

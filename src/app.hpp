#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.hpp"
#include "Lights.hpp"
#include "Model.hpp"
#include "Particle.hpp"
#include "SceneObject.hpp"
#include "ShaderProgram.hpp"
#include "Texture.hpp"

// ============================================================
// EnemyState — physics + AI state for the chasing sphere.
//
// Kept here rather than in its own header because it is only
// ever used by App.  If the enemy grows more complex, extract
// it then.
// ============================================================
struct EnemyState {
    glm::vec3 position   {-4.0f, 0.75f, -4.0f};
    glm::vec3 velocity   {0.0f};
    float     radius     {0.75f};
    float     hitCooldown{0.0f};

    static constexpr float SPEED = 1.5f;  // world units / second
};

// ============================================================
// App — owns all OpenGL state, the scene, and the game loop.
// ============================================================
class App {
public:
    App();
    ~App();

    // Entry points called by main()
    bool init();
    int  run();

    // ---- Window state (read by GLFW callbacks) ----
    GLFWwindow* window     {nullptr};
    bool isFullScreen      {false};
    int  savedXPos{0}, savedYPos{0};
    int  savedWidth{800},  savedHeight{600};
    bool show_imgui        {true};
    bool vsync             {true};

    // ---- Asset libraries ----
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> shader_library;
    std::unordered_map<std::string, std::shared_ptr<Texture>>       texture_library;

    // ---- Named scene models (kept public so callbacks can reach them) ----
    std::shared_ptr<Model> bunny_model;   // untextured opaque (Task 08)
    std::shared_ptr<Model> sphere_model;  // textured  opaque (Task 08)
    std::shared_ptr<Model> ground_model;  // ground plane
    std::shared_ptr<Model> enemy_model;   // Task 09 — collision target

    // Task 09 — Task 1: painter's-algorithm transparent objects
    std::vector<TransparentObject> transparent_objects;

    // Task 09 — Task 2: collision
    EnemyState enemy;
    static constexpr float CAMERA_RADIUS = 0.4f;  // player bounding sphere
    static constexpr float MAP_HALF_SIZE = 12.0f;  // XZ map boundary ±n
    static constexpr float FLOOR_Y       = 0.0f;   // ground plane height

    // Task 09 — Task 3: particles
    ParticleSystem particle_system;

    // ---- Lighting (public so ImGui / callbacks can tweak them) ----
    DirectionalLight          dirLight;
    std::array<PointLight, 3> pointLights;
    SpotLight                 spotLight;

    // ---- Window helpers ----
    void         toggle_fullscreen(GLFWwindow* win);
    GLFWmonitor* GetCurrentMonitor(GLFWwindow* win);

    // ---- GLFW callbacks (static — access App via glfwGetWindowUserPointer) ----
    static void glfw_error_callback(int error, const char* description);
    static void glfw_framebuffer_size_callback(GLFWwindow*, int width, int height);
    static void glfw_key_callback(GLFWwindow*, int key, int scancode, int action, int mods);
    static void glfw_mouse_button_callback(GLFWwindow*, int button, int action, int mods);
    static void glfw_cursor_position_callback(GLFWwindow*, double xpos, double ypos);
    static void glfw_scroll_callback(GLFWwindow*, double xoffset, double yoffset);

private:
    // ---- Viewport & projection ----
    int       width{800}, height{600};
    float     fov{60.0f};
    glm::mat4 projection_matrix{glm::identity<glm::mat4>()};

    // ---- Camera (initial position set here — no need to repeat in init()) ----
    Camera camera{glm::vec3(0.0f, 1.5f, 8.0f)};
    double cursorLastX{0.0}, cursorLastY{0.0};
    double last_frame_time{0.0};

    // ---- FPS accounting ----
    double lastTime   {0.0};
    int    frameCount {0};

    // ---- Initialisation (each method owns one concern) ----
    void initOpenGL();
    void init_imgui();
    void init_assets();    // calls the four below in order
    void init_shaders();
    void init_textures();
    void init_scene();
    void init_lighting();
    void update_projection_matrix();

    // ---- Per-frame rendering helpers ----

    // Draw a model with Phong lighting uniforms.
    // alpha = 1.0 for opaque, model->alpha for transparent.
    void draw_model_lit(const std::shared_ptr<Model>& model,
                        const Material& material,
                        const glm::mat4& view,
                        float alpha = 1.0f);

    // Upload all light uniforms to a shader (view-space transform applied here).
    void set_lighting_uniforms(const std::shared_ptr<ShaderProgram>& shader,
                               const glm::mat4& view_matrix) const;
};

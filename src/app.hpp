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
#include "Level.hpp"
#include "Lights.hpp"
#include "Model.hpp"
#include "Particle.hpp"
#include "Player.hpp"
#include "SceneObject.hpp"   // Material struct
#include "ShaderProgram.hpp"
#include "Texture.hpp"

// GamePhase — controls which logic/HUD runs each frame.
enum class GamePhase { Playing, Won };

// App — owns all OpenGL state, scene, physics, and game loop.
class App {
public:
    App();
    ~App();

    bool init();
    int  run();

    // ── Window state (read by GLFW callbacks) ──────────────────
    GLFWwindow* window    {nullptr};
    bool isFullScreen     {false};
    int  savedXPos{0}, savedYPos{0};
    int  savedWidth{800},  savedHeight{600};
    bool show_imgui       {true};
    bool vsync            {false};
    bool msaa             {false};
    bool imgui_initialized{false};

    // ── Asset libraries ────────────────────────────────────────
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> shader_library;
    std::unordered_map<std::string, std::shared_ptr<Texture>>       texture_library;

    // ── Game state ─────────────────────────────────────────────
    GamePhase phase         {GamePhase::Playing};
    float     elapsed_time  {0.0f};   // seconds since level start
    float     completion_time{0.0f};  // time at the moment of winning
    int       death_count   {0};

    // ── Core game objects ──────────────────────────────────────
    Level  level;
    Player player;

    // ── Lighting ───────────────────────────────────────────────
    DirectionalLight          dirLight;
    PointLights               pointLights;
    SpotLight                 spotLight;

    // ── GLFW callbacks ─────────────────────────────────────────
    static void glfw_error_callback           (int, const char*);
    static void glfw_framebuffer_size_callback(GLFWwindow*, int, int);
    static void glfw_key_callback             (GLFWwindow*, int, int, int, int);
    static void glfw_mouse_button_callback    (GLFWwindow*, int, int, int);
    static void glfw_cursor_position_callback (GLFWwindow*, double, double);
    static void glfw_scroll_callback          (GLFWwindow*, double, double);

private:
    // ── Viewport & projection ──────────────────────────────────
    int       width{800}, height{600};
    float     fov  {70.0f};
    glm::mat4 projection_matrix{glm::identity<glm::mat4>()};

    // ── Camera (position is kept in sync with player.eye_pos()) ─
    Camera camera{glm::vec3(0.0f, 2.1f, 0.0f)};
    double cursorLastX{0.0}, cursorLastY{0.0};
    double last_frame_time{0.0};
    double lastTime   {0.0};
    int    frameCount {0};

    // ── Block rendering ────────────────────────────────────────
    // One shared mesh per BlockType; each block just supplies a model matrix.
    std::unordered_map<BlockType, std::shared_ptr<Mesh>> block_meshes;

    // ── Spatial partitioning for collision detection ──────────────
    // Grid maps global cell keys to block pointers for fast spatial lookups.
    std::unordered_map<long long, std::vector<const LevelBlock*>> collision_grid;
    static constexpr int GRID_CELL_SIZE = 16;  // blocks per cell
    void build_collision_grid();

    // ── OBJ-loaded scene decoration (satisfy "2 models from file") ─
    std::shared_ptr<Model> deco_orb;     // orbiting sphere loaded from OBJ
    std::shared_ptr<Model> trophy_bunny; // spinning bunny at the end
    float orb_angle   {0.0f};
    float bunny_spin  {0.0f};

    // ── Particle system ────────────────────────────────────────
    ParticleSystem particle_system;

    // ── Initialisation ─────────────────────────────────────────
    void initOpenGL();
    void init_imgui();
    void init_assets();
    void init_shaders();
    void init_textures();
    void init_scene();
    void init_lighting();
    void update_projection_matrix();

    // ── Per-frame update ───────────────────────────────────────
    void update_physics(float dt);
    void resolve_collisions(float prev_feet_y);
    void respawn();

    // ── Per-frame render ───────────────────────────────────────
    void draw_scene(const glm::mat4& view);
    void draw_block(const LevelBlock& block, const glm::mat4& view, float alpha = 1.0f);
    void draw_model_lit(const std::shared_ptr<Model>&, const Material&,
                        const glm::mat4& view, float alpha = 1.0f);

    void set_lighting_uniforms(const std::shared_ptr<ShaderProgram>&,
                               const glm::mat4& view) const;

    // ── HUD helpers ────────────────────────────────────────────
    void draw_hud();
    // ── Helper functions ──────────────────────────────────
    void load_config(std::string filename);
    void save_screenshot(void);
    void         toggle_fullscreen(GLFWwindow*);
    GLFWmonitor* GetCurrentMonitor(GLFWwindow*);
    // Config struct for loading/saving settings
    struct WindowConfig {
        int width{800};
        int height{600};
        std::string title{"OpenGL Window"};
        bool vsync{false};
        bool msaa{false};
    };
    WindowConfig config;
};

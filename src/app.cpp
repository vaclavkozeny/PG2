// app.cpp — Minecraft Parkour Game
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include <opencv4/opencv2/opencv.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "app.hpp"
#include "fps_meter.hpp"
#include "gl_err_callback.hpp"

// ============================================================
// Block materials — one per BlockType, used by draw_block().
// ============================================================
namespace {

const Material& material_for(BlockType t) {
    static const Material GRASS  { glm::vec3(0.15f,0.30f,0.10f), glm::vec3(0.40f,0.85f,0.30f), glm::vec3(0.05f),              8.0f  };
    static const Material DIRT   { glm::vec3(0.25f,0.18f,0.10f), glm::vec3(0.70f,0.50f,0.30f), glm::vec3(0.05f),              4.0f  };
    static const Material STONE  { glm::vec3(0.20f),             glm::vec3(0.75f),              glm::vec3(0.10f),              8.0f  };
    static const Material COBBLE { glm::vec3(0.20f),             glm::vec3(0.70f),              glm::vec3(0.12f),              8.0f  };
    static const Material PLANKS { glm::vec3(0.30f,0.25f,0.15f), glm::vec3(0.90f,0.75f,0.50f), glm::vec3(0.10f),              8.0f  };
    static const Material SAND   { glm::vec3(0.30f,0.28f,0.20f), glm::vec3(0.90f,0.85f,0.60f), glm::vec3(0.05f),              4.0f  };
    static const Material LOG    { glm::vec3(0.25f,0.20f,0.10f), glm::vec3(0.80f,0.65f,0.40f), glm::vec3(0.10f),              8.0f  };
    static const Material GOLD   { glm::vec3(0.30f,0.25f,0.05f), glm::vec3(1.00f,0.90f,0.20f), glm::vec3(0.90f,0.80f,0.30f), 64.0f };
    static const Material GLASS  { glm::vec3(0.05f,0.15f,0.15f), glm::vec3(0.10f,0.90f,0.90f), glm::vec3(0.95f),              128.0f};
    switch (t) {
        case BlockType::Grass:       return GRASS;
        case BlockType::Dirt:        return DIRT;
        case BlockType::Stone:       return STONE;
        case BlockType::Cobblestone: return COBBLE;
        case BlockType::Planks:      return PLANKS;
        case BlockType::Sand:        return SAND;
        case BlockType::Log:         return LOG;
        case BlockType::Gold:        return GOLD;
        case BlockType::Glass:       return GLASS;
    }
    return STONE;
}

} // namespace

// ============================================================
// Constructor / Destructor
// ============================================================

App::App() { std::cout << "Constructed...\n"; }

App::~App() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (window) { glfwDestroyWindow(window); window = nullptr; }
    glfwTerminate();
    cv::destroyAllWindows();
    std::cout << "Bye...\n";
}

// ============================================================
// OpenGL / GLFW bootstrap
// ============================================================

void App::initOpenGL() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) throw std::runtime_error("GLFW init failed.");

    glfwWindowHint(GLFW_VISIBLE,               GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES,               4);  // MSAA 4×

    window = glfwCreateWindow(800, 600, "Minecraft Parkour", nullptr, nullptr);
    if (!window) throw std::runtime_error("Window creation failed.");

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
    glfwSetMouseButtonCallback    (window, glfw_mouse_button_callback);
    glfwSetKeyCallback            (window, glfw_key_callback);
    glfwSetScrollCallback         (window, glfw_scroll_callback);
    glfwSetCursorPosCallback      (window, glfw_cursor_position_callback);

    glewExperimental = GL_TRUE;
    glewInit();
}

void App::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
    std::cout << "ImGui " << ImGui::GetVersion() << "\n";
}

bool App::init() {
    try {
        initOpenGL();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);
        glClearColor(0.45f, 0.65f, 0.88f, 1.0f);  // sky blue

        if (GLEW_ARB_debug_output) {
            glDebugMessageCallback(MessageCallback, nullptr);
            glEnable(GL_DEBUG_OUTPUT);
            std::cout << "GL_DEBUG enabled.\n";
        }

        glfwSwapInterval(vsync ? 1 : 0);

        init_assets();
        init_imgui();

        glfwGetFramebufferSize(window, &width, &height);
        if (height < 1) height = 1;
        glViewport(0, 0, width, height);
        update_projection_matrix();

        glfwGetCursorPos(window, &cursorLastX, &cursorLastY);
        glfwShowWindow(window);
    }
    catch (const std::exception& e) {
        std::cerr << "Init failed: " << e.what() << "\n";
        throw;
    }
    std::cout << "Initialized." << std::endl;
    return true;
}

// ============================================================
// Asset init
// ============================================================

void App::init_assets() {
    init_shaders();
    init_textures();
    init_scene();
    init_lighting();
    std::cout << "Assets ready." << std::endl;
}

void App::init_shaders() {
    auto load = [&](const std::string& name, const std::string& v, const std::string& f) {
        shader_library.emplace(name, std::make_shared<ShaderProgram>(
            std::filesystem::path(v), std::filesystem::path(f)));
    };
    load("lighting", "./lighting.vert", "./lighting.frag");
    load("particle",  "./particle.vert",  "./particle.frag");
}

void App::init_textures() {
    // tex_256.png = Minecraft terrain atlas (16×16 tiles).
    // Use nearest-neighbour filter for crisp pixel-art look.
    texture_library.emplace("atlas",
        std::make_shared<Texture>(
            std::filesystem::path("../resources/textures/tex_256.png"),
            Texture::Interpolation::nearest));

    // Solid white: diffuse color drives appearance (for OBJ models without atlas UVs)
    texture_library.emplace("white", std::make_shared<Texture>(glm::vec3(1.0f)));
    // Solid box texture for the decorative orb
    texture_library.emplace("box",
        std::make_shared<Texture>(
            std::filesystem::path("../resources/textures/box.png")));
}

void App::init_scene() {
    auto& lit = shader_library.at("lighting");

    // ── Build one shared mesh per BlockType ───────────────────
    for (const auto& [type, def] : block_defs())
        block_meshes.emplace(type, make_block_mesh(def));

    // ── Load level from JSON ──────────────────────────────────
    level.load(std::filesystem::path("../resources/level.json"));

    // Position player at level start, facing +X (course direction)
    camera.Position = level.start_eye_pos;
    camera.SetOrientation(0.0f);
    player.feet     = level.start_eye_pos - glm::vec3(0.0f, Player::EYE_HEIGHT, 0.0f);

    // ── Decorative OBJ models (required: ≥2 loaded from file) ─

    // Orbiting sphere above the start of the course
    try {
        deco_orb = std::make_shared<Model>(
            std::filesystem::path("../obj_samples/sphere_tri_vnt.obj"), lit);
        deco_orb->setScale(glm::vec3(1.2f));
        deco_orb->meshes[0].texture = texture_library.at("box");
        std::cout << "Loaded decorative sphere.\n";
    } catch (const std::exception& e) {
        std::cerr << "Could not load sphere: " << e.what() << "\n";
    }

    // Spinning trophy bunny above the end platform
    try {
        trophy_bunny = std::make_shared<Model>(
            std::filesystem::path("../obj_samples/bunny_tri_vn.obj"), lit);
        trophy_bunny->setScale(glm::vec3(0.07f));
        trophy_bunny->setPosition(glm::vec3(38.0f, 10.0f, 0.0f));
        trophy_bunny->meshes[0].texture = texture_library.at("white");
        std::cout << "Loaded trophy bunny." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Could not load bunny: " << e.what() << "\n";
    }

    // ── Particle mesh: tetrahedron (shared by all particles) ──
    {
        std::vector<Vertex> tv{
            {{ 1.0f,  1.0f,  1.0f}, {0,0,0}, {0,0}},
            {{ 1.0f, -1.0f, -1.0f}, {0,0,0}, {0,0}},
            {{-1.0f,  1.0f, -1.0f}, {0,0,0}, {0,0}},
            {{-1.0f, -1.0f,  1.0f}, {0,0,0}, {0,0}},
        };
        std::vector<GLuint> ti{0, 1, 2, 3};
        particle_system.init(std::make_shared<Mesh>(tv, ti, GL_TRIANGLE_STRIP));
    }
}

void App::init_lighting() {
    // ── Directional (animated sun) ────────────────────────────
    dirLight.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    dirLight.ambient   = glm::vec3(0.06f);
    dirLight.diffuse   = glm::vec3(1.0f, 0.95f, 0.8f);
    dirLight.specular  = glm::vec3(1.0f, 0.97f, 0.85f);

    // ── Point lights — orbit different sections of the course ─

    // Red/warm — orbits near the start (x≈10)
    pointLights[0].diffuse      = glm::vec3(1.0f, 0.35f, 0.10f);
    pointLights[0].specular     = glm::vec3(1.0f, 0.35f, 0.10f);
    pointLights[0].ambient      = glm::vec3(0.02f, 0.007f, 0.002f);
    pointLights[0].orbitCenter  = glm::vec3(10.0f, 0.0f, 0.0f);
    pointLights[0].orbitRadius  = 6.0f;
    pointLights[0].orbitHeight  = 4.0f;
    pointLights[0].orbitSpeed   = 0.8f;
    pointLights[0].orbitAngle   = 0.0f;
    pointLights[0].linear       = 0.045f;
    pointLights[0].quadratic    = 0.0075f;

    // Blue/cool — orbits the middle section (x≈22)
    pointLights[1].diffuse      = glm::vec3(0.20f, 0.45f, 1.0f);
    pointLights[1].specular     = glm::vec3(0.20f, 0.45f, 1.0f);
    pointLights[1].ambient      = glm::vec3(0.004f, 0.009f, 0.02f);
    pointLights[1].orbitCenter  = glm::vec3(22.0f, 0.0f, 0.0f);
    pointLights[1].orbitRadius  = 8.0f;
    pointLights[1].orbitHeight  = 6.0f;
    pointLights[1].orbitSpeed   = 0.5f;
    pointLights[1].orbitAngle   = 2.09f;
    pointLights[1].linear       = 0.035f;
    pointLights[1].quadratic    = 0.005f;

    // Gold/warm — orbits near the end platform (x≈38)
    pointLights[2].diffuse      = glm::vec3(1.0f, 0.85f, 0.20f);
    pointLights[2].specular     = glm::vec3(1.0f, 0.85f, 0.20f);
    pointLights[2].ambient      = glm::vec3(0.02f, 0.017f, 0.004f);
    pointLights[2].orbitCenter  = glm::vec3(38.0f, 0.0f, 0.0f);
    pointLights[2].orbitRadius  = 5.0f;
    pointLights[2].orbitHeight  = 5.0f;
    pointLights[2].orbitSpeed   = 1.2f;
    pointLights[2].orbitAngle   = 4.19f;
    pointLights[2].linear       = 0.045f;
    pointLights[2].quadratic    = 0.0075f;

    // ── Spotlight — camera headlight ──────────────────────────
    spotLight.direction   = glm::vec3(0.0f, 0.0f, -1.0f);
    spotLight.ambient     = glm::vec3(0.0f);
    spotLight.diffuse     = glm::vec3(1.0f, 1.0f, 0.9f);
    spotLight.specular    = glm::vec3(1.0f, 1.0f, 0.9f);
    spotLight.cutoff      = std::cos(glm::radians(12.5f));
    spotLight.outerCutoff = std::cos(glm::radians(17.5f));
    spotLight.constant    = 1.0f;
    spotLight.linear      = 0.045f;
    spotLight.quadratic   = 0.0075f;
    spotLight.on          = true;
}

// ============================================================
// Lighting upload
// ============================================================

void App::set_lighting_uniforms(const std::shared_ptr<ShaderProgram>& shader,
                                const glm::mat4& view) const
{
    const glm::mat3 V3(view);

    shader->setUniform("dirLight.direction", V3 * dirLight.direction);
    shader->setUniform("dirLight.ambient",   dirLight.ambient);
    shader->setUniform("dirLight.diffuse",   dirLight.diffuse);
    shader->setUniform("dirLight.specular",  dirLight.specular);

    for (int i = 0; i < 3; ++i) {
        const std::string p = "pointLights[" + std::to_string(i) + "].";
        const glm::vec3 posV = glm::vec3(view * glm::vec4(pointLights[i].worldPosition(), 1.0f));
        shader->setUniform(p + "position",  posV);
        shader->setUniform(p + "ambient",   pointLights[i].ambient);
        shader->setUniform(p + "diffuse",   pointLights[i].diffuse);
        shader->setUniform(p + "specular",  pointLights[i].specular);
        shader->setUniform(p + "constant",  pointLights[i].constant);
        shader->setUniform(p + "linear",    pointLights[i].linear);
        shader->setUniform(p + "quadratic", pointLights[i].quadratic);
    }

    shader->setUniform("spotLight.direction",   spotLight.direction);
    shader->setUniform("spotLight.ambient",     spotLight.ambient);
    shader->setUniform("spotLight.diffuse",     spotLight.diffuse);
    shader->setUniform("spotLight.specular",    spotLight.specular);
    shader->setUniform("spotLight.cutoff",      spotLight.cutoff);
    shader->setUniform("spotLight.outerCutoff", spotLight.outerCutoff);
    shader->setUniform("spotLight.constant",    spotLight.constant);
    shader->setUniform("spotLight.linear",      spotLight.linear);
    shader->setUniform("spotLight.quadratic",   spotLight.quadratic);
    shader->setUniform("spotLightOn",           spotLight.on ? 1 : 0);
}

// ============================================================
// draw_block — draws one block instance from the shared mesh.
// ============================================================

void App::draw_block(const LevelBlock& block, const glm::mat4& view, float alpha) {
    auto& shader = shader_library.at("lighting");
    shader->use();  // no-op if already active; ensures correct shader for draw call
    const Material& mat = material_for(block.type);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(block.grid));

    shader->setUniform("uP_m", projection_matrix);
    shader->setUniform("uV_m", view);
    shader->setUniform("uM_m", model);

    shader->setUniform("mat_ambient",   mat.ambient);
    shader->setUniform("mat_diffuse",   mat.diffuse);
    shader->setUniform("mat_specular",  mat.specular);
    shader->setUniform("mat_shininess", mat.shininess);
    shader->setUniform("mat_alpha",     alpha);

    texture_library.at("atlas")->bind(0);
    shader->setUniform("tex0", 0);

    block_meshes.at(block.type)->draw();
}

// ============================================================
// draw_model_lit — Phong-lit draw for OBJ models.
// ============================================================

void App::draw_model_lit(const std::shared_ptr<Model>& mdl,
                         const Material& mat,
                         const glm::mat4& view,
                         float alpha)
{
    if (!mdl) return;
    auto& shader = shader_library.at("lighting");
    for (const auto& pkg : mdl->meshes) {
        shader->use();
        shader->setUniform("uP_m", projection_matrix);
        shader->setUniform("uV_m", view);

        const glm::mat4 localM = Model::createModelMatrix(
            pkg.origin, pkg.eulerAngles, pkg.scale);
        shader->setUniform("uM_m", mdl->getModelMatrix() * localM);

        shader->setUniform("mat_ambient",   mat.ambient);
        shader->setUniform("mat_diffuse",   mat.diffuse);
        shader->setUniform("mat_specular",  mat.specular);
        shader->setUniform("mat_shininess", mat.shininess);
        shader->setUniform("mat_alpha",     alpha);

        if (pkg.texture) { pkg.texture->bind(0); shader->setUniform("tex0", 0); }
        pkg.mesh->draw();
    }
}

// ============================================================
// draw_scene — renders opaque blocks, OBJ models, then the
// transparent (glass) blocks and particles in the blended pass.
// ============================================================

void App::draw_scene(const glm::mat4& view) {
    auto& shader = shader_library.at("lighting");
    shader->use();
    set_lighting_uniforms(shader, view);

    // ── Pass 1: Opaque blocks ──────────────────────────────────
    for (const auto& block : level.blocks) {
        if (block_defs().at(block.type).transparent) continue;
        draw_block(block, view);
    }

    // ── Opaque OBJ models ──────────────────────────────────────
    static const Material MAT_ORB  { glm::vec3(0.15f), glm::vec3(0.8f), glm::vec3(0.7f), 64.0f  };
    static const Material MAT_BUNNY{ glm::vec3(0.15f), glm::vec3(0.8f), glm::vec3(0.1f), 16.0f };
    draw_model_lit(deco_orb,     MAT_ORB,   view);
    draw_model_lit(trophy_bunny, MAT_BUNNY, view);

    // ── Pass 2: Transparent blocks + particles (blended) ───────
    {
        // Collect transparent blocks and sort back-to-front
        std::vector<const LevelBlock*> glass_blocks;
        for (const auto& block : level.blocks)
            if (block_defs().at(block.type).transparent)
                glass_blocks.push_back(&block);

        std::sort(glass_blocks.begin(), glass_blocks.end(),
            [&](const LevelBlock* a, const LevelBlock* b) {
                return glm::distance(camera.Position, glm::vec3(a->grid))
                     > glm::distance(camera.Position, glm::vec3(b->grid));
            });

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        for (const LevelBlock* b : glass_blocks) {
            const float alpha = block_defs().at(b->type).alpha;
            draw_block(*b, view, alpha);
        }

        if (!particle_system.empty())
            particle_system.draw(*shader_library.at("particle"), view, projection_matrix);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

// ============================================================
// Player physics
// ============================================================

void App::update_physics(float dt) {
    // ── Horizontal input (camera-relative, projected to XZ) ────
    glm::vec3 forward = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
    glm::vec3 right   = glm::normalize(glm::vec3(camera.Right.x, 0.0f, camera.Right.z));

    glm::vec3 h_move{0.0f};
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) h_move += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) h_move -= forward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) h_move -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) h_move += right;
    if (glm::length(h_move) > 0.01f)
        h_move = glm::normalize(h_move) * Player::MOVE_SPEED;

    // ── Jump ───────────────────────────────────────────────────
    if (player.jump_req) {
        if (player.grounded) {
            player.vel_y    = Player::JUMP_SPEED;
            player.grounded = false;
            // Small puff of dust on jump
            particle_system.emit(player.feet,
                glm::vec3(0.55f, 0.45f, 0.30f), 6, 1.8f, 0.35f);
        }
        player.jump_req = false;
    }

    // ── Gravity ────────────────────────────────────────────────
    if (!player.grounded)
        player.vel_y += Player::GRAVITY * dt;

    // ── Vertical movement → resolve Y collisions ───────────────
    const float prev_feet_y = player.feet.y;
    player.feet.y += player.vel_y * dt;

    player.grounded = false;
    resolve_collisions(prev_feet_y);

    // ── Horizontal movement → re-resolve XZ collisions ─────────
    player.feet.x += h_move.x * dt;
    player.feet.z += h_move.z * dt;

    // Re-resolve only XZ after horizontal move (pass same prev_feet_y
    // so the Y branch won't fire again for the same blocks).
    resolve_collisions(prev_feet_y);

    // ── Sync camera to player eye ──────────────────────────────
    camera.Position = player.eye_pos();
}

void App::resolve_collisions(float prev_feet_y) {
    const float R  = Player::RADIUS;
    const float H  = Player::EYE_HEIGHT;

    for (const auto& block : level.blocks) {
        const glm::vec3 bp{block.grid};
        const float bx1 = bp.x - 0.5f, bx2 = bp.x + 0.5f;
        const float by1 = bp.y - 0.5f, by2 = bp.y + 0.5f;
        const float bz1 = bp.z - 0.5f, bz2 = bp.z + 0.5f;

        const float px1 = player.feet.x - R, px2 = player.feet.x + R;
        const float py1 = player.feet.y,     py2 = player.feet.y + H;
        const float pz1 = player.feet.z - R, pz2 = player.feet.z + R;

        // Quick AABB rejection
        if (px2 <= bx1 || px1 >= bx2) continue;
        if (py2 <= by1 || py1 >= by2) continue;
        if (pz2 <= bz1 || pz1 >= bz2) continue;

        // Determine resolution axis from previous position
        const bool came_from_above = prev_feet_y        >= by2 - 0.02f;
        const bool came_from_below = prev_feet_y + H    <= by1 + 0.02f;

        if (came_from_above && player.vel_y <= 0.01f) {
            // Landing on top of block
            player.feet.y   = by2;
            player.vel_y    = 0.0f;
            player.grounded = true;
        } else if (came_from_below && player.vel_y > 0.0f) {
            // Head hit ceiling
            player.feet.y = by1 - H;
            player.vel_y  = 0.0f;
        } else {
            // Horizontal push-out (smallest XZ penetration)
            const float pen_x = std::min(px2 - bx1, bx2 - px1);
            const float pen_z = std::min(pz2 - bz1, bz2 - pz1);
            if (pen_x < pen_z) {
                player.feet.x = (player.feet.x < bp.x) ? bx1 - R : bx2 + R;
            } else {
                player.feet.z = (player.feet.z < bp.z) ? bz1 - R : bz2 + R;
            }
        }
    }
}

void App::respawn() {
    camera.Position = level.start_eye_pos;
    camera.SetOrientation(0.0f);
    player.feet     = level.start_eye_pos - glm::vec3(0.0f, Player::EYE_HEIGHT, 0.0f);
    player.vel_y    = 0.0f;
    player.grounded = false;
    player.jump_req = false;
    ++death_count;
}

// ============================================================
// HUD — ImGui overlay
// ============================================================

void App::draw_hud() {
    if (!show_imgui) return;

    // ── Playing HUD (top-left timer + controls) ───────────────
    if (phase == GamePhase::Playing) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::Begin("##hud", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);

        int mins = static_cast<int>(elapsed_time) / 60;
        float secs = elapsed_time - mins * 60.0f;
        ImGui::Text("Time  %02d:%05.2f", mins, secs);
        ImGui::Text("Deaths  %d", death_count);
        ImGui::Separator();
        ImGui::Text("FPS  %.0f", ImGui::GetIO().Framerate);
        ImGui::Separator();
        ImGui::Text("WASD = move   Space = jump");
        ImGui::Text("H = headlight   I = HUD   F = fullscreen");
        ImGui::Text("Right-click = release cursor");
        ImGui::End();
    }

    // ── Win screen (centred overlay) ──────────────────────────
    if (phase == GamePhase::Won) {
        ImVec2 centre(width * 0.5f, height * 0.5f);
        ImGui::SetNextWindowPos(centre, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowBgAlpha(0.85f);
        ImGui::Begin("##win", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);

        ImGui::SetWindowFontScale(1.8f);
        ImGui::TextColored(ImVec4(1,0.85f,0.1f,1), "Course Complete!");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Separator();

        int mins = static_cast<int>(completion_time) / 60;
        float secs = completion_time - mins * 60.0f;
        ImGui::Text("Time:   %02d:%05.2f", mins, secs);
        ImGui::Text("Deaths: %d", death_count);
        ImGui::Separator();
        ImGui::Text("Press R to restart   ESC to quit");
        ImGui::End();
    }
}

// ============================================================
// Main loop
// ============================================================

int App::run() {
    try {
        lastTime   = glfwGetTime();
        frameCount = 0;
        fps_meter  FPS;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            const double now = glfwGetTime();
            const float  dt  = static_cast<float>(now - last_frame_time);
            last_frame_time  = now;
            ++frameCount;

            // ── State update ───────────────────────────────────

            // Animate directional (sun) light
            {
                const float angle = static_cast<float>(now) * 0.20f;
                const float elev  = std::sin(angle);
                dirLight.direction = glm::normalize(glm::vec3(
                    std::cos(angle), -std::abs(elev) - 0.1f, std::sin(angle)));
                const float t = glm::clamp(std::abs(elev), 0.0f, 1.0f);
                const glm::vec3 sunColor = glm::mix(
                    glm::vec3(1.0f, 0.4f, 0.1f),
                    glm::vec3(1.0f, 0.97f, 0.88f), t);
                const float bright = glm::clamp(std::abs(elev) * 1.3f, 0.08f, 1.0f);
                dirLight.diffuse  = sunColor * bright * 0.85f;
                dirLight.specular = sunColor * bright * 0.95f;
                dirLight.ambient  = sunColor * 0.05f;
            }

            // Advance point-light orbits
            for (auto& pl : pointLights)
                pl.orbitAngle += dt * pl.orbitSpeed;

            // Animate decorative models
            orb_angle  += dt * 0.4f;
            bunny_spin += dt * 90.0f;  // degrees/s

            if (deco_orb) {
                // Orbits above the course at height 8, radius 6 around x=19
                glm::vec3 centre{19.0f, 0.0f, 0.0f};
                deco_orb->setPosition(centre + glm::vec3(
                    6.0f * std::cos(orb_angle), 8.0f, 6.0f * std::sin(orb_angle)));
            }
            if (trophy_bunny) {
                trophy_bunny->setEulerAngles(glm::vec3(0.0f, bunny_spin, 0.0f));
            }

            // Game-phase-specific updates
            if (phase == GamePhase::Playing) {
                elapsed_time += dt;

                update_physics(dt);
                particle_system.update(dt);

                if (player.is_dead()) {
                    respawn();
                }

                if (level.at_end(camera.Position)) {
                    phase = GamePhase::Won;
                    completion_time = elapsed_time;
                    // Celebration firework burst
                    particle_system.emit(
                        glm::vec3(38.0f, 9.5f, 0.0f),
                        glm::vec3(1.0f, 0.85f, 0.1f), 60, 6.0f, 2.5f);
                    std::cout << "Completed in " << completion_time << "s!\n";
                }
            } else {
                // In win state still advance particles
                particle_system.update(dt);
            }

            // ── Render ────────────────────────────────────────
            const glm::mat4 view = camera.GetViewMatrix();

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ImGui frame start
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            draw_scene(view);
            draw_hud();

            // FPS window title
            if (now - lastTime >= 1.0) {
                const double fps = frameCount / (now - lastTime);
                glfwSetWindowTitle(window,
                    ("Minecraft Parkour — FPS: " + std::to_string(int(fps))).c_str());
                frameCount = 0;
                lastTime   = now;
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
            FPS.update();
            if (FPS.is_updated())
                std::cout << "FPS: " << FPS.get() << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "App failed: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// ============================================================
// Helpers
// ============================================================

void App::update_projection_matrix() {
    if (height < 1) height = 1;
    projection_matrix = glm::perspective(
        glm::radians(fov),
        static_cast<float>(width) / height,
        0.05f, 2000.0f);
}

void App::toggle_fullscreen(GLFWwindow* win) {
    if (isFullScreen) {
        glfwSetWindowMonitor(win, nullptr,
            savedXPos, savedYPos, savedWidth, savedHeight, GLFW_DONT_CARE);
        isFullScreen = false;
    } else {
        glfwGetWindowPos (win, &savedXPos,  &savedYPos);
        glfwGetWindowSize(win, &savedWidth, &savedHeight);
        GLFWmonitor*       mon  = GetCurrentMonitor(win);
        const GLFWvidmode* mode = glfwGetVideoMode(mon);
        glfwSetWindowMonitor(win, mon, 0, 0, mode->width, mode->height, mode->refreshRate);
        isFullScreen = true;
    }
}

GLFWmonitor* App::GetCurrentMonitor(GLFWwindow* win) {
    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    if (!monitors) return glfwGetPrimaryMonitor();

    int wx, wy, ww, wh;
    glfwGetWindowPos (win, &wx, &wy);
    glfwGetWindowSize(win, &ww, &wh);

    GLFWmonitor* best = nullptr;
    int bestArea = 0;

    for (int i = 0; i < count; ++i) {
        int mx, my;
        glfwGetMonitorPos(monitors[i], &mx, &my);
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
        if (!mode) continue;
        const int ox   = std::max(0, std::min(wx+ww, mx+mode->width)  - std::max(wx, mx));
        const int oy   = std::max(0, std::min(wy+wh, my+mode->height) - std::max(wy, my));
        const int area = ox * oy;
        if (area > bestArea) { bestArea = area; best = monitors[i]; }
    }
    return best ? best : glfwGetPrimaryMonitor();
}

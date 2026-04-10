// app.cpp
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
#include "assets.hpp"
#include "fps_meter.hpp"
#include "gl_err_callback.hpp"

// ============================================================
// File-internal material constants.
//
// Defined once here instead of re-constructing them every frame.
// Adding a new material: add an entry in this block and reference
// it by name in draw calls below.
// ============================================================
namespace {
    const Material MAT_GROUND {
        glm::vec3(0.25f),
        glm::vec3(0.65f),
        glm::vec3(0.05f),
        4.0f
    };
    const Material MAT_BUNNY {
        glm::vec3(0.15f),
        glm::vec3(0.85f),
        glm::vec3(0.10f),
        16.0f
    };
    const Material MAT_SPHERE {
        glm::vec3(0.15f),
        glm::vec3(0.90f),
        glm::vec3(0.70f),
        64.0f
    };
    const Material MAT_ENEMY {
        glm::vec3(0.20f, 0.04f, 0.04f),
        glm::vec3(0.95f, 0.10f, 0.10f),
        glm::vec3(0.80f, 0.30f, 0.30f),
        32.0f
    };
} // namespace

// --------------------------------------------------------------------------
// Constructor / Destructor
// --------------------------------------------------------------------------

App::App() {
    std::cout << "Constructed...\n";
}

App::~App() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
    cv::destroyAllWindows();
    std::cout << "Bye...\n";
}

// --------------------------------------------------------------------------
// OpenGL / GLFW bootstrap
// --------------------------------------------------------------------------

void App::initOpenGL() {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())
        throw std::runtime_error("GLFW can not be initialized.");

    glfwWindowHint(GLFW_VISIBLE,              GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(800, 600, "OpenGL context", nullptr, nullptr);
    if (!window)
        throw std::runtime_error("Window creation failed.");

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
    std::cout << "ImGUI version: " << ImGui::GetVersion() << "\n";
}

bool App::init() {
    try {
        initOpenGL();

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

        if (GLEW_ARB_debug_output) {
            glDebugMessageCallback(MessageCallback, nullptr);
            glEnable(GL_DEBUG_OUTPUT);
            std::cout << "GL_DEBUG enabled.\n";
        } else {
            std::cout << "GL_DEBUG NOT SUPPORTED!\n";
        }

        glfwSwapInterval(vsync ? 1 : 0);

        init_assets();
        init_imgui();

        glfwGetFramebufferSize(window, &width, &height);
        if (height <= 0) height = 1;
        glViewport(0, 0, width, height);
        update_projection_matrix();

        // Capture the initial cursor position so the first mouse event
        // doesn't produce a large spurious delta.
        glfwGetCursorPos(window, &cursorLastX, &cursorLastY);

        glfwShowWindow(window);
    }
    catch (const std::exception& e) {
        std::cerr << "Init failed: " << e.what() << "\n";
        throw;
    }
    std::cout << "Initialized...\n";
    return true;
}

// --------------------------------------------------------------------------
// Asset initialisation — split by concern
// --------------------------------------------------------------------------

void App::init_assets() {
    init_shaders();
    init_textures();
    init_scene();
    init_lighting();
    std::cout << "Assets initialized...\n";
}

void App::init_shaders() {
    auto load = [&](const std::string& name,
                    const std::string& vert,
                    const std::string& frag)
    {
        shader_library.emplace(name, std::make_shared<ShaderProgram>(
            std::filesystem::path(vert),
            std::filesystem::path(frag)
        ));
    };

    load("simple_shader",  "./basic.vert",    "./basic.frag");
    load("texture_shader", "./tex.vert",      "./tex.frag");
    load("lighting",       "./lighting.vert", "./lighting.frag");
    load("particle",       "./particle.vert", "./particle.frag");
}

void App::init_textures() {
    auto load_file = [&](const std::string& name, const std::string& path) {
        texture_library.emplace(name,
            std::make_shared<Texture>(std::filesystem::path(path)));
    };

    load_file("box",    "../resources/textures/box.png");
    load_file("tex256", "../resources/textures/tex_256.png");

    // Solid white: lets the material diffuse color drive appearance
    // when a model has no meaningful UV texture.
    texture_library.emplace("white", std::make_shared<Texture>(glm::vec3(1.0f)));
}

void App::init_scene() {
    auto& lit     = shader_library.at("lighting");
    auto& white   = texture_library.at("white");
    auto& box_tex = texture_library.at("box");
    auto& tile    = texture_library.at("tex256");

    // ---- Ground plane (24×24, tiled 4×) ----
    {
        const float H = MAP_HALF_SIZE;
        std::vector<Vertex> verts{
            {{-H, 0.0f, -H}, {0,1,0}, {0.0f, 0.0f}},
            {{ H, 0.0f, -H}, {0,1,0}, {4.0f, 0.0f}},
            {{-H, 0.0f,  H}, {0,1,0}, {0.0f, 4.0f}},
            {{ H, 0.0f,  H}, {0,1,0}, {4.0f, 4.0f}},
        };
        // Indices: two CCW triangles → normal points +Y
        std::vector<GLuint> idx{0, 2, 1, 1, 2, 3};
        auto mesh = std::make_shared<Mesh>(verts, idx, GL_TRIANGLES);
        ground_model = std::make_shared<Model>();
        ground_model->addMesh(mesh, lit, tile);
    }

    // ---- Bunny (opaque, white diffuse) ----
    try {
        bunny_model = std::make_shared<Model>(
            std::filesystem::path("../obj_samples/bunny_tri_vn.obj"), lit);
        bunny_model->setScale(glm::vec3(0.05f));
        bunny_model->setPosition(glm::vec3(0.175f, 0.0f, -1.05f));
        bunny_model->meshes[0].texture = white;
        std::cout << "Loaded bunny\n";
    } catch (const std::exception& e) {
        std::cerr << "Could not load bunny: " << e.what() << "\n";
    }

    // ---- Main textured sphere (opaque, box texture) ----
    try {
        sphere_model = std::make_shared<Model>(
            std::filesystem::path("../obj_samples/sphere_tri_vnt.obj"), lit);
        sphere_model->setScale(glm::vec3(1.5f));
        sphere_model->setPosition(glm::vec3(3.5f, 1.5f, 0.0f)); // y=radius: sits on floor
        sphere_model->meshes[0].texture = box_tex;
        std::cout << "Loaded textured sphere\n";
    } catch (const std::exception& e) {
        std::cerr << "Could not load textured sphere: " << e.what() << "\n";
    }

    // ---- Enemy sphere (opaque, red — Task 2) ----
    try {
        enemy_model = std::make_shared<Model>(
            std::filesystem::path("../obj_samples/sphere_tri_vnt.obj"), lit);
        enemy_model->setScale(glm::vec3(enemy.radius));
        enemy_model->setPosition(enemy.position);
        enemy_model->meshes[0].texture = white;
        std::cout << "Loaded enemy sphere\n";
    } catch (const std::exception& e) {
        std::cerr << "Could not load enemy: " << e.what() << "\n";
    }

    // ---- Transparent glass spheres (Task 1) ----
    //
    // Each entry: world position, uniform scale, alpha, ambient color, diffuse color.
    // To add more: append to this table — no other code needs changing.
    struct GlassSpec { glm::vec3 pos; float scale, alpha; glm::vec3 amb, diff; };
    const GlassSpec specs[] = {
        {{ 2.0f, 0.5f,-3.0f}, 0.5f, 0.45f, {0.04f,0.18f,0.18f}, {0.10f,0.90f,0.90f}}, // cyan
        {{-2.0f, 0.8f, 2.0f}, 0.8f, 0.40f, {0.18f,0.04f,0.18f}, {0.90f,0.10f,0.90f}}, // magenta
        {{ 0.5f, 1.2f, 4.5f}, 1.2f, 0.35f, {0.18f,0.18f,0.04f}, {0.90f,0.85f,0.10f}}, // gold
    };
    for (const auto& s : specs) {
        try {
            auto sphere = std::make_shared<Model>(
                std::filesystem::path("../obj_samples/sphere_tri_vnt.obj"), lit);
            sphere->setScale(glm::vec3(s.scale));
            sphere->setPosition(s.pos);
            sphere->meshes[0].texture = white;
            sphere->is_transparent    = true;
            sphere->alpha             = s.alpha;

            Material mat;
            mat.ambient   = s.amb;
            mat.diffuse   = s.diff;
            mat.specular  = glm::vec3(0.95f);
            mat.shininess = 128.0f;

            transparent_objects.push_back({sphere, mat});
            std::cout << "Loaded glass sphere at ("
                      << s.pos.x << ',' << s.pos.y << ',' << s.pos.z << ")\n";
        } catch (const std::exception& e) {
            std::cerr << "Could not load glass sphere: " << e.what() << "\n";
        }
    }

    // ---- Particle mesh: regular tetrahedron (Task 3) ----
    //
    // Four vertices → GL_TRIANGLE_STRIP gives 2 visible triangles — enough
    // for small fast-moving particles (README spec: "four vertices forming
    // GL_TRIANGLE_STRIP").
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
    // ---- Directional (sun) — animated in run() ----
    dirLight.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    dirLight.ambient   = glm::vec3(0.04f);
    dirLight.diffuse   = glm::vec3(1.0f, 0.9f, 0.7f);
    dirLight.specular  = glm::vec3(1.0f, 0.95f, 0.8f);

    // ---- Point lights — each orbits at a different radius/height/speed ----
    // Red/warm — fast, high
    pointLights[0].diffuse     = glm::vec3(1.0f, 0.25f, 0.1f);
    pointLights[0].specular    = glm::vec3(1.0f, 0.25f, 0.1f);
    pointLights[0].ambient     = glm::vec3(0.02f, 0.005f, 0.002f);
    pointLights[0].orbitRadius = 4.0f;
    pointLights[0].orbitHeight = 1.5f;
    pointLights[0].orbitSpeed  = 1.4f;
    pointLights[0].orbitAngle  = 0.0f;
    pointLights[0].linear      = 0.07f;
    pointLights[0].quadratic   = 0.017f;

    // Blue/cool — medium, mid-height (120° offset)
    pointLights[1].diffuse     = glm::vec3(0.2f, 0.45f, 1.0f);
    pointLights[1].specular    = glm::vec3(0.2f, 0.45f, 1.0f);
    pointLights[1].ambient     = glm::vec3(0.004f, 0.009f, 0.02f);
    pointLights[1].orbitRadius = 3.0f;
    pointLights[1].orbitHeight = 0.8f;
    pointLights[1].orbitSpeed  = 0.8f;
    pointLights[1].orbitAngle  = 2.09f;  // 2π/3
    pointLights[1].linear      = 0.09f;
    pointLights[1].quadratic   = 0.032f;

    // Green — slow, wide (240° offset)
    pointLights[2].diffuse     = glm::vec3(0.15f, 1.0f, 0.3f);
    pointLights[2].specular    = glm::vec3(0.15f, 1.0f, 0.3f);
    pointLights[2].ambient     = glm::vec3(0.003f, 0.02f, 0.006f);
    pointLights[2].orbitRadius = 5.5f;
    pointLights[2].orbitHeight = 1.0f;
    pointLights[2].orbitSpeed  = 0.5f;
    pointLights[2].orbitAngle  = 4.19f;  // 4π/3
    pointLights[2].linear      = 0.045f;
    pointLights[2].quadratic   = 0.0075f;

    // ---- Spotlight — camera headlight (H to toggle) ----
    spotLight.direction   = glm::vec3(0.0f, 0.0f, -1.0f); // view-space -Z
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

// --------------------------------------------------------------------------
// Lighting uniform upload
// --------------------------------------------------------------------------

void App::set_lighting_uniforms(const std::shared_ptr<ShaderProgram>& shader,
                                const glm::mat4& view_matrix) const
{
    const glm::mat3 V3(view_matrix); // upper-left 3×3 for direction transforms

    // Directional (view-space direction)
    shader->setUniform("dirLight.direction", V3 * dirLight.direction);
    shader->setUniform("dirLight.ambient",   dirLight.ambient);
    shader->setUniform("dirLight.diffuse",   dirLight.diffuse);
    shader->setUniform("dirLight.specular",  dirLight.specular);

    // Point lights (view-space positions)
    for (int i = 0; i < 3; ++i) {
        const std::string p = "pointLights[" + std::to_string(i) + "].";
        const glm::vec3 posV = glm::vec3(view_matrix * glm::vec4(pointLights[i].worldPosition(), 1.0f));
        shader->setUniform(p + "position",  posV);
        shader->setUniform(p + "ambient",   pointLights[i].ambient);
        shader->setUniform(p + "diffuse",   pointLights[i].diffuse);
        shader->setUniform(p + "specular",  pointLights[i].specular);
        shader->setUniform(p + "constant",  pointLights[i].constant);
        shader->setUniform(p + "linear",    pointLights[i].linear);
        shader->setUniform(p + "quadratic", pointLights[i].quadratic);
    }

    // Spotlight (camera = view-space origin, dir = -Z)
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

// --------------------------------------------------------------------------
// draw_model_lit — uploads per-object Phong uniforms and draws all meshes.
//
// alpha = 1.0  → opaque pass
// alpha < 1.0  → transparent pass (blending must already be enabled by caller)
// --------------------------------------------------------------------------

void App::draw_model_lit(const std::shared_ptr<Model>& mdl,
                         const Material& mat,
                         const glm::mat4& view,
                         float alpha)
{
    if (!mdl) return;
    for (const auto& pkg : mdl->meshes) {
        pkg.shader->use();
        pkg.shader->setUniform("uP_m", projection_matrix);
        pkg.shader->setUniform("uV_m", view);

        const glm::mat4 localM = Model::createModelMatrix(
            pkg.origin, pkg.eulerAngles, pkg.scale);
        pkg.shader->setUniform("uM_m", mdl->getModelMatrix() * localM);

        pkg.shader->setUniform("mat_ambient",   mat.ambient);
        pkg.shader->setUniform("mat_diffuse",   mat.diffuse);
        pkg.shader->setUniform("mat_specular",  mat.specular);
        pkg.shader->setUniform("mat_shininess", mat.shininess);
        pkg.shader->setUniform("mat_alpha",     alpha);

        if (pkg.texture) {
            pkg.texture->bind(0);
            pkg.shader->setUniform("tex0", 0);
        }
        pkg.mesh->draw();
    }
}

// --------------------------------------------------------------------------
// Main loop
// --------------------------------------------------------------------------

int App::run() {
    try {
        lastTime   = glfwGetTime();
        frameCount = 0;
        fps_meter FPS;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            const double currentTime = glfwGetTime();
            const float  dt          = static_cast<float>(currentTime - last_frame_time);
            last_frame_time = currentTime;
            ++frameCount;

            // ============================================================
            // State updates
            // ============================================================

            // Animate sun: direction rotates, color shifts warm↔cool with elevation
            {
                const float angle = static_cast<float>(currentTime) * 0.25f;
                const float elev  = std::sin(angle);
                dirLight.direction = glm::normalize(glm::vec3(
                    std::cos(angle),
                    -std::abs(elev) - 0.1f,
                    std::sin(angle)
                ));
                const float t = glm::clamp(std::abs(elev), 0.0f, 1.0f);
                const glm::vec3 sunColor = glm::mix(
                    glm::vec3(1.0f, 0.4f, 0.1f),   // horizon — warm orange
                    glm::vec3(1.0f, 0.97f, 0.88f),  // zenith  — cool white
                    t
                );
                const float bright = glm::clamp(std::abs(elev) * 1.2f, 0.05f, 1.0f);
                dirLight.diffuse  = sunColor * bright * 0.8f;
                dirLight.specular = sunColor * bright * 0.9f;
                dirLight.ambient  = sunColor * 0.04f;
            }

            // Advance point-light orbits
            for (auto& pl : pointLights)
                pl.orbitAngle += dt * pl.orbitSpeed;

            // Enemy AI: chase camera (or glide back after a hit)
            {
                if (enemy.hitCooldown > 0.0f) {
                    enemy.hitCooldown -= dt;
                    enemy.position    += enemy.velocity * dt;
                    enemy.velocity    *= std::pow(0.05f, dt); // exponential decel
                } else {
                    glm::vec3 toCamera = camera.Position - enemy.position;
                    toCamera.y = 0.0f;
                    if (glm::length(toCamera) > 0.05f)
                        enemy.velocity = glm::normalize(toCamera) * EnemyState::SPEED;
                    enemy.position += enemy.velocity * dt;
                }
                enemy.position.y = enemy.radius;  // always on the floor
                enemy.position.x = glm::clamp(enemy.position.x, -MAP_HALF_SIZE, MAP_HALF_SIZE);
                enemy.position.z = glm::clamp(enemy.position.z, -MAP_HALF_SIZE, MAP_HALF_SIZE);

                if (enemy_model)
                    enemy_model->setPosition(enemy.position);
            }

            // Camera movement with floor + wall-sliding collision (Task 2 — col. 1)
            {
                glm::vec3 delta  = camera.ProcessInput(window, dt);
                glm::vec3 newPos = camera.Position + delta;

                // Floor: camera sphere cannot descend below FLOOR_Y
                newPos.y = glm::max(newPos.y, FLOOR_Y + CAMERA_RADIUS);

                // Walls: if the new position exits the map boundary on one
                // axis, keep the old value for that axis only — the player
                // can still slide along the wall on the other axis.
                const float wall = MAP_HALF_SIZE - CAMERA_RADIUS;
                if (newPos.x < -wall || newPos.x > wall) newPos.x = camera.Position.x;
                if (newPos.z < -wall || newPos.z > wall) newPos.z = camera.Position.z;

                camera.Position = newPos;
            }

            // Camera ↔ enemy sphere collision (Task 2 — col. 2)
            {
                const float dist = glm::distance(camera.Position, enemy.position);
                if (dist < CAMERA_RADIUS + enemy.radius && enemy.hitCooldown <= 0.0f) {
                    // Push camera out of the enemy sphere
                    const glm::vec3 push = glm::normalize(camera.Position - enemy.position);
                    camera.Position = enemy.position + push * (CAMERA_RADIUS + enemy.radius + 0.1f);
                    camera.Position.y = glm::max(camera.Position.y, FLOOR_Y + CAMERA_RADIUS);

                    // Emit orange particle burst at the contact point (Task 3)
                    const glm::vec3 contact = enemy.position + push * enemy.radius;
                    particle_system.emit(contact, glm::vec3(1.0f, 0.5f, 0.05f), 25, 4.5f, 1.8f);

                    // Bounce the enemy back with a brief cooldown
                    glm::vec3 away = -push;
                    away.y = 0.0f;
                    enemy.velocity    = away * 3.5f;
                    enemy.hitCooldown = 2.5f;

                    std::cout << "Hit! Particles: " << particle_system.count() << "\n";
                }
            }

            // Advance particle physics
            particle_system.update(dt);

            // ============================================================
            // Render
            // ============================================================

            const glm::mat4 view = camera.GetViewMatrix();

            // Light uniforms are set once per frame using glProgramUniform*,
            // which works without the shader being currently bound.
            set_lighting_uniforms(shader_library.at("lighting"), view);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ---- ImGui ----
            if (show_imgui) {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::Begin("Scene");
                ImGui::Text("FPS: %.1f",  ImGui::GetIO().Framerate);
                ImGui::Text("Camera: (%.1f, %.1f, %.1f)",
                    camera.Position.x, camera.Position.y, camera.Position.z);
                ImGui::Text("Enemy:  (%.1f, %.1f, %.1f)  CD: %.1f",
                    enemy.position.x, enemy.position.y, enemy.position.z,
                    enemy.hitCooldown);
                ImGui::Text("Particles: %zu", particle_system.count());
                ImGui::Separator();
                bool headlight = spotLight.on;
                if (ImGui::Checkbox("[H] Headlight", &headlight))
                    spotLight.on = headlight;
                ImGui::Separator();
                ImGui::Text("WASD + Space/Ctrl = move");
                ImGui::Text("Walk into red sphere = collision!");
                ImGui::End();
            }

            // Window title (once per second)
            if (currentTime - lastTime >= 1.0) {
                const double fps = frameCount / (currentTime - lastTime);
                glfwSetWindowTitle(window,
                    ("Alpha + Physics Demo — FPS: " +
                     std::to_string(static_cast<int>(fps))).c_str());
                frameCount = 0;
                lastTime   = currentTime;
            }

            // ============================================================
            // Pass 1 — Opaque geometry
            // Default GL state: depth write ON, depth test ON, blend OFF
            // ============================================================
            draw_model_lit(ground_model,  MAT_GROUND, view);
            draw_model_lit(bunny_model,   MAT_BUNNY,  view);
            draw_model_lit(sphere_model,  MAT_SPHERE, view);
            draw_model_lit(enemy_model,   MAT_ENEMY,  view);

            // ============================================================
            // Pass 2 — Transparent objects (Task 1) + particles (Task 3)
            //
            // Rules applied here (from lecture / sample alpha.cpp):
            //  • Sort transparent objects back-to-front (painter's algorithm)
            //    so closer glass composites on top of farther glass.
            //  • glDepthMask(GL_FALSE): keep reading depth (transparent
            //    objects hide behind opaque), stop writing depth (transparent
            //    objects don't occlude each other erroneously).
            //  • glBlendFunc(SRC_ALPHA, 1-SRC_ALPHA): standard alpha blend.
            //  • Particles drawn in the same pass (same GL state).
            //  • Restore depth mask and disable blend before the next frame.
            // ============================================================
            {
                // Sort a local copy — the stored order is the add-order,
                // not the render order, so we sort a cheap copy of shared_ptrs.
                auto sorted = transparent_objects;
                std::sort(sorted.begin(), sorted.end(),
                    [&](const TransparentObject& a, const TransparentObject& b) {
                        return glm::distance(camera.Position, a.model->getPosition())
                             > glm::distance(camera.Position, b.model->getPosition());
                    });

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);

                for (const auto& obj : sorted)
                    draw_model_lit(obj.model, obj.material, view, obj.model->alpha);

                // Particles share the blended pass (they are also transparent)
                if (!particle_system.empty())
                    particle_system.draw(*shader_library.at("particle"), view, projection_matrix);

                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
            }

            // ---- Finalize ImGui ----
            if (show_imgui) {
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            if (FPS.is_updated())
                std::cout << "FPS: " << FPS.get() << "\n";

            glfwSwapBuffers(window);
            FPS.update();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "App failed: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "Finished OK...\n";
    return EXIT_SUCCESS;
}

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------

void App::update_projection_matrix() {
    if (height < 1) height = 1;
    projection_matrix = glm::perspective(
        glm::radians(fov),
        static_cast<float>(width) / height,
        0.1f,
        20000.0f
    );
}

void App::toggle_fullscreen(GLFWwindow* win) {
    if (isFullScreen) {
        glfwSetWindowMonitor(win, nullptr,
            savedXPos, savedYPos, savedWidth, savedHeight, GLFW_DONT_CARE);
        isFullScreen = false;
    } else {
        glfwGetWindowPos (win, &savedXPos,   &savedYPos);
        glfwGetWindowSize(win, &savedWidth,  &savedHeight);
        GLFWmonitor*       monitor = GetCurrentMonitor(win);
        const GLFWvidmode* mode    = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(win, monitor, 0, 0,
            mode->width, mode->height, mode->refreshRate);
        isFullScreen = true;
    }
}

GLFWmonitor* App::GetCurrentMonitor(GLFWwindow* win) {
    int monitorCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    if (!monitors) return glfwGetPrimaryMonitor();

    int wx, wy, ww, wh;
    glfwGetWindowPos (win, &wx, &wy);
    glfwGetWindowSize(win, &ww, &wh);

    GLFWmonitor* best     = nullptr;
    int          bestArea = 0;

    for (int i = 0; i < monitorCount; ++i) {
        int mx, my;
        glfwGetMonitorPos(monitors[i], &mx, &my);
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
        if (!mode) continue;

        const int ox   = std::max(0, std::min(wx + ww, mx + mode->width)  - std::max(wx, mx));
        const int oy   = std::max(0, std::min(wy + wh, my + mode->height) - std::max(wy, my));
        const int area = ox * oy;
        if (area > bestArea) { bestArea = area; best = monitors[i]; }
    }
    return best ? best : glfwGetPrimaryMonitor();
}

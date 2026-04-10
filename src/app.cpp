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
#include "gl_err_callback.hpp"
#include "fps_meter.hpp"

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
// Initialization
// --------------------------------------------------------------------------

void App::initOpenGL() {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())
        throw std::runtime_error("GLFW can not be initialized.");

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    // macOS requires Core Profile 4.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(800, 600, "OpenGL context", nullptr, nullptr);
    if (!window)
        throw std::runtime_error("Window creation failed.");

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
    glfwSetMouseButtonCallback(window,     glfw_mouse_button_callback);
    glfwSetKeyCallback(window,             glfw_key_callback);
    glfwSetScrollCallback(window,          glfw_scroll_callback);
    glfwSetCursorPosCallback(window,       glfw_cursor_position_callback);

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

        glfwGetCursorPos(window, &cursorLastX, &cursorLastY);

        camera.Position = glm::vec3(0.0f, 1.5f, 8.0f);

        glfwShowWindow(window);
    }
    catch (std::exception const& e) {
        std::cerr << "Init failed: " << e.what() << "\n";
        throw;
    }
    std::cout << "Initialized...\n";
    return true;
}

void App::init_assets() {
    // ---- Shaders ----
    shader_library.emplace("simple_shader", std::make_shared<ShaderProgram>(
        std::filesystem::path("./basic.vert"),
        std::filesystem::path("./basic.frag")
    ));
    shader_library.emplace("texture_shader", std::make_shared<ShaderProgram>(
        std::filesystem::path("./tex.vert"),
        std::filesystem::path("./tex.frag")
    ));
    shader_library.emplace("lighting", std::make_shared<ShaderProgram>(
        std::filesystem::path("./lighting.vert"),
        std::filesystem::path("./lighting.frag")
    ));
    // Task 3: Unlit particle shader
    shader_library.emplace("particle", std::make_shared<ShaderProgram>(
        std::filesystem::path("./particle.vert"),
        std::filesystem::path("./particle.frag")
    ));

    // ---- Textures ----
    texture_library.emplace("box",
        std::make_shared<Texture>(std::filesystem::path("../resources/textures/box.png")));
    texture_library.emplace("tex256",
        std::make_shared<Texture>(std::filesystem::path("../resources/textures/tex_256.png")));
    // Solid white: used when the material color should drive appearance
    texture_library.emplace("white",
        std::make_shared<Texture>(glm::vec3(1.0f)));

    // ---- Ground plane (large textured quad at y=0) ----
    {
        // Tiled 4x texture coords for visual interest
        const float H = MAP_HALF_SIZE;
        std::vector<Vertex> verts{
            {{-H, 0.0f, -H}, {0,1,0}, {0.0f, 0.0f}},
            {{ H, 0.0f, -H}, {0,1,0}, {4.0f, 0.0f}},
            {{-H, 0.0f,  H}, {0,1,0}, {0.0f, 4.0f}},
            {{ H, 0.0f,  H}, {0,1,0}, {4.0f, 4.0f}},
        };
        // Two CCW triangles — normal points +Y
        std::vector<GLuint> idx{0, 2, 1, 1, 2, 3};
        auto mesh = std::make_shared<Mesh>(verts, idx, GL_TRIANGLES);
        ground = std::make_shared<Model>();
        ground->addMesh(mesh, shader_library.at("lighting"), texture_library.at("tex256"));
    }

    // ---- Bunny (opaque, white diffuse) ----
    try {
        model = std::make_shared<Model>(
            std::filesystem::path("../obj_samples/bunny_tri_vn.obj"),
            shader_library.at("lighting")
        );
        model->setScale(glm::vec3(0.05f));
        model->setPosition(glm::vec3(0.175f, 0.0f, -1.05f));
        model->meshes[0].texture = texture_library.at("white");
        std::cout << "Loaded bunny\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Could not load bunny: " << e.what() << "\n";
    }

    // ---- Main textured sphere (opaque, box texture) ----
    try {
        textured_model = std::make_shared<Model>(
            std::filesystem::path("../obj_samples/sphere_tri_vnt.obj"),
            shader_library.at("lighting")
        );
        textured_model->setScale(glm::vec3(1.5f));
        textured_model->setPosition(glm::vec3(3.5f, 1.5f, 0.0f)); // raised to sit on ground
        textured_model->meshes[0].texture = texture_library.at("box");
        std::cout << "Loaded textured sphere\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Could not load textured sphere: " << e.what() << "\n";
    }

    // ---- Enemy sphere (opaque, red) — Task 2 ----
    try {
        enemy_model = std::make_shared<Model>(
            std::filesystem::path("../obj_samples/sphere_tri_vnt.obj"),
            shader_library.at("lighting")
        );
        enemy_model->setScale(glm::vec3(enemy.radius));
        enemy_model->setPosition(enemy.position);
        enemy_model->meshes[0].texture = texture_library.at("white");
        std::cout << "Loaded enemy sphere\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Could not load enemy sphere: " << e.what() << "\n";
    }

    // ---- Task 1: Three semi-transparent glass spheres ----
    //
    // Each has is_transparent=true, alpha<1, and a distinctive glass color.
    // They are rendered in a second pass using the painter's algorithm
    // (sorted back-to-front from the camera, drawn with blending enabled
    //  and depth writes disabled).
    //
    struct GlassCfg {
        glm::vec3 pos;
        float     scale;
        float     alpha;
        glm::vec3 ambient;
        glm::vec3 diffuse;
    };
    const GlassCfg glassCfgs[] = {
        // Cyan glass sphere
        {{ 2.0f, 0.5f, -3.0f}, 0.5f, 0.45f, {0.04f, 0.18f, 0.18f}, {0.1f, 0.9f, 0.9f}},
        // Magenta glass sphere
        {{-2.0f, 0.8f,  2.0f}, 0.8f, 0.40f, {0.18f, 0.04f, 0.18f}, {0.9f, 0.1f, 0.9f}},
        // Gold glass sphere
        {{ 0.5f, 1.2f,  4.5f}, 1.2f, 0.35f, {0.18f, 0.18f, 0.04f}, {0.9f, 0.85f, 0.1f}},
    };

    for (const auto& cfg : glassCfgs) {
        try {
            auto sphere = std::make_shared<Model>(
                std::filesystem::path("../obj_samples/sphere_tri_vnt.obj"),
                shader_library.at("lighting")
            );
            sphere->setScale(glm::vec3(cfg.scale));
            sphere->setPosition(cfg.pos);
            sphere->meshes[0].texture = texture_library.at("white");
            sphere->is_transparent    = true;
            sphere->alpha             = cfg.alpha;

            Material mat;
            mat.ambient   = cfg.ambient;
            mat.diffuse   = cfg.diffuse;
            mat.specular  = glm::vec3(0.95f);  // very shiny glass
            mat.shininess = 128.0f;

            transparent_objects.push_back({sphere, mat});
            std::cout << "Loaded glass sphere at ("
                      << cfg.pos.x << "," << cfg.pos.y << "," << cfg.pos.z << ")\n";
        }
        catch (const std::exception& e) {
            std::cerr << "Could not load glass sphere: " << e.what() << "\n";
        }
    }

    // ---- Task 3: Particle system — tetrahedron mesh ----
    //
    // A regular tetrahedron has 4 vertices: each pair of vertices is the same
    // distance apart. With GL_TRIANGLE_STRIP and 4 vertices we get 2 visible
    // triangles, which is sufficient for small fast-moving particles.
    {
        std::vector<Vertex> tv{
            {{ 1.0f,  1.0f,  1.0f}, {0,0,0}, {0,0}},
            {{ 1.0f, -1.0f, -1.0f}, {0,0,0}, {0,0}},
            {{-1.0f,  1.0f, -1.0f}, {0,0,0}, {0,0}},
            {{-1.0f, -1.0f,  1.0f}, {0,0,0}, {0,0}},
        };
        std::vector<GLuint> ti{0, 1, 2, 3};
        auto tetraMesh = std::make_shared<Mesh>(tv, ti, GL_TRIANGLE_STRIP);
        particle_system.init(tetraMesh);
    }

    // ---- Lighting setup ----

    // Directional light — animated sun
    dirLight.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    dirLight.ambient   = glm::vec3(0.04f);
    dirLight.diffuse   = glm::vec3(1.0f, 0.9f, 0.7f);
    dirLight.specular  = glm::vec3(1.0f, 0.95f, 0.8f);

    // Point light 0 — red/warm, fast orbit at height 1.5
    pointLights[0].diffuse      = glm::vec3(1.0f, 0.25f, 0.1f);
    pointLights[0].specular     = glm::vec3(1.0f, 0.25f, 0.1f);
    pointLights[0].ambient      = glm::vec3(0.02f, 0.005f, 0.002f);
    pointLights[0].orbitRadius  = 4.0f;
    pointLights[0].orbitHeight  = 1.5f;
    pointLights[0].orbitSpeed   = 1.4f;
    pointLights[0].orbitAngle   = 0.0f;
    pointLights[0].linear       = 0.07f;
    pointLights[0].quadratic    = 0.017f;

    // Point light 1 — blue/cool, medium orbit at height 0
    pointLights[1].diffuse      = glm::vec3(0.2f, 0.45f, 1.0f);
    pointLights[1].specular     = glm::vec3(0.2f, 0.45f, 1.0f);
    pointLights[1].ambient      = glm::vec3(0.004f, 0.009f, 0.02f);
    pointLights[1].orbitRadius  = 3.0f;
    pointLights[1].orbitHeight  = 0.8f;
    pointLights[1].orbitSpeed   = 0.8f;
    pointLights[1].orbitAngle   = 2.09f; // 120° offset
    pointLights[1].linear       = 0.09f;
    pointLights[1].quadratic    = 0.032f;

    // Point light 2 — green, slow wide orbit at height -0.5
    pointLights[2].diffuse      = glm::vec3(0.15f, 1.0f, 0.3f);
    pointLights[2].specular     = glm::vec3(0.15f, 1.0f, 0.3f);
    pointLights[2].ambient      = glm::vec3(0.003f, 0.02f, 0.006f);
    pointLights[2].orbitRadius  = 5.5f;
    pointLights[2].orbitHeight  = 1.0f;
    pointLights[2].orbitSpeed   = 0.5f;
    pointLights[2].orbitAngle   = 4.19f; // 240° offset
    pointLights[2].linear       = 0.045f;
    pointLights[2].quadratic    = 0.0075f;

    // Spotlight — camera headlight (H to toggle)
    spotLight.direction   = glm::vec3(0.0f, 0.0f, -1.0f);
    spotLight.ambient     = glm::vec3(0.0f);
    spotLight.diffuse     = glm::vec3(1.0f, 1.0f, 0.9f);
    spotLight.specular    = glm::vec3(1.0f, 1.0f, 0.9f);
    spotLight.cutoff      = std::cos(glm::radians(12.5f));
    spotLight.outerCutoff = std::cos(glm::radians(20.0f));
    spotLight.constant    = 1.0f;
    spotLight.linear      = 0.045f;
    spotLight.quadratic   = 0.0075f;
    spotLight.on          = true;

    std::cout << "Assets initialized...\n";
}

// --------------------------------------------------------------------------
// Lighting uniform upload
// --------------------------------------------------------------------------

void App::set_lighting_uniforms(const std::shared_ptr<ShaderProgram>& shader,
                                const glm::mat4& view_matrix) const
{
    glm::mat3 V3(view_matrix);

    // Directional light
    shader->setUniform("dirLight.direction", V3 * dirLight.direction);
    shader->setUniform("dirLight.ambient",   dirLight.ambient);
    shader->setUniform("dirLight.diffuse",   dirLight.diffuse);
    shader->setUniform("dirLight.specular",  dirLight.specular);

    // Point lights
    for (int i = 0; i < 3; i++) {
        const std::string p = "pointLights[" + std::to_string(i) + "].";
        glm::vec3 posV = glm::vec3(view_matrix * glm::vec4(pointLights[i].currentWorldPos(), 1.0f));
        shader->setUniform(p + "position",  posV);
        shader->setUniform(p + "ambient",   pointLights[i].ambient);
        shader->setUniform(p + "diffuse",   pointLights[i].diffuse);
        shader->setUniform(p + "specular",  pointLights[i].specular);
        shader->setUniform(p + "constant",  pointLights[i].constant);
        shader->setUniform(p + "linear",    pointLights[i].linear);
        shader->setUniform(p + "quadratic", pointLights[i].quadratic);
    }

    // Spotlight (camera-space: pos=origin, dir=(0,0,-1))
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
// Main loop
// --------------------------------------------------------------------------

int App::run() {
    try {
        lastTime   = glfwGetTime();
        frameCount = 0;
        fps_meter FPS;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            double currentTime = glfwGetTime();
            double deltaTime   = currentTime - last_frame_time;
            last_frame_time    = currentTime;
            const float dt     = static_cast<float>(deltaTime);
            frameCount++;

            // ============================================================
            // Update: directional light (animated sun)
            // ============================================================
            {
                float sunAngle  = static_cast<float>(currentTime) * 0.25f;
                float elevation = std::sin(sunAngle);
                dirLight.direction = glm::normalize(glm::vec3(
                    std::cos(sunAngle),
                    -std::abs(elevation) - 0.1f,
                    std::sin(sunAngle)
                ));
                float t = glm::clamp(std::abs(elevation), 0.0f, 1.0f);
                glm::vec3 sunColor = glm::mix(
                    glm::vec3(1.0f, 0.4f, 0.1f),   // horizon: warm orange
                    glm::vec3(1.0f, 0.97f, 0.88f),  // zenith: cool white
                    t
                );
                float bright = glm::clamp(std::abs(elevation) * 1.2f, 0.05f, 1.0f);
                dirLight.diffuse  = sunColor * bright * 0.8f;
                dirLight.specular = sunColor * bright * 0.9f;
                dirLight.ambient  = sunColor * 0.04f;
            }

            // Update point light orbits
            for (auto& pl : pointLights)
                pl.orbitAngle += dt * pl.orbitSpeed;

            // ============================================================
            // Update: enemy chase (Task 2 — collision 2)
            // ============================================================
            {
                if (enemy.hitCooldown > 0.0f) {
                    enemy.hitCooldown -= dt;
                    // Glide away, decelerating quickly
                    enemy.position += enemy.velocity * dt;
                    enemy.velocity *= std::pow(0.05f, dt); // exponential decel
                } else {
                    // Chase the camera (project direction onto XZ plane)
                    glm::vec3 toCamera = camera.Position - enemy.position;
                    toCamera.y = 0.0f;
                    float len = glm::length(toCamera);
                    if (len > 0.05f)
                        enemy.velocity = glm::normalize(toCamera) * EnemyState::SPEED;
                    enemy.position += enemy.velocity * dt;
                }
                // Keep enemy on the floor and inside the map
                enemy.position.y = enemy.radius;
                enemy.position.x = glm::clamp(enemy.position.x, -MAP_HALF_SIZE, MAP_HALF_SIZE);
                enemy.position.z = glm::clamp(enemy.position.z, -MAP_HALF_SIZE, MAP_HALF_SIZE);

                if (enemy_model)
                    enemy_model->setPosition(enemy.position);
            }

            // ============================================================
            // Update: camera movement with collision (Task 2 — collision 1)
            // ============================================================
            {
                glm::vec3 delta  = camera.ProcessInput(window, dt);
                glm::vec3 newPos = camera.Position + delta;

                // Floor collision — camera can't descend below floor
                const float minY = FLOOR_Y + CAMERA_RADIUS;
                if (newPos.y < minY) newPos.y = minY;

                // Wall sliding — if the new position in X is out of bounds,
                // keep the old X so the player can still move in Z (slides
                // along the wall instead of stopping completely).
                const float wall = MAP_HALF_SIZE - CAMERA_RADIUS;
                if (newPos.x < -wall || newPos.x > wall)
                    newPos.x = camera.Position.x;
                if (newPos.z < -wall || newPos.z > wall)
                    newPos.z = camera.Position.z;

                camera.Position = newPos;
            }

            // ============================================================
            // Collision detection: camera-enemy sphere (Task 2 — collision 2)
            // ============================================================
            {
                float dist = glm::distance(camera.Position, enemy.position);
                if (dist < CAMERA_RADIUS + enemy.radius && enemy.hitCooldown <= 0.0f) {

                    // Push the camera out of the enemy sphere
                    glm::vec3 pushDir  = glm::normalize(camera.Position - enemy.position);
                    camera.Position    = enemy.position + pushDir * (CAMERA_RADIUS + enemy.radius + 0.1f);
                    // Re-apply floor constraint after push
                    if (camera.Position.y < FLOOR_Y + CAMERA_RADIUS)
                        camera.Position.y = FLOOR_Y + CAMERA_RADIUS;

                    // Task 3: Emit orange particle burst at collision point
                    glm::vec3 contactPt = enemy.position + pushDir * enemy.radius;
                    particle_system.emit(contactPt, glm::vec3(1.0f, 0.5f, 0.05f), 25, 4.5f, 1.8f);

                    // Bounce enemy back and put it on cooldown
                    glm::vec3 away = -pushDir;
                    away.y         = 0.0f;
                    enemy.velocity    = away * 3.5f;
                    enemy.hitCooldown = 2.5f;

                    std::cout << "Enemy hit! Particles: " << particle_system.count() << "\n";
                }
            }

            // Task 3: Advance particle simulation
            particle_system.update(dt);

            // ============================================================
            // Render preparation
            // ============================================================

            glm::mat4 view_matrix = camera.GetViewMatrix();

            // Upload lighting uniforms once (glProgramUniform* = no bind needed)
            set_lighting_uniforms(shader_library.at("lighting"), view_matrix);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ============================================================
            // ImGui
            // ============================================================
            if (show_imgui) {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::Begin("Scene");
                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                ImGui::Text("Camera: (%.1f, %.1f, %.1f)",
                    camera.Position.x, camera.Position.y, camera.Position.z);
                ImGui::Text("Enemy:  (%.1f, %.1f, %.1f)  CD: %.1f",
                    enemy.position.x, enemy.position.y, enemy.position.z, enemy.hitCooldown);
                ImGui::Text("Particles: %zu", particle_system.count());
                ImGui::Separator();

                ImGui::Text("[H] Spotlight (headlight)");
                bool headlight = spotLight.on;
                if (ImGui::Checkbox("Headlight ON", &headlight))
                    spotLight.on = headlight;

                ImGui::Text("[Mouse wheel] FOV: %.0f", fov);
                ImGui::Separator();
                ImGui::Text("WASD + Space/Ctrl = move");
                ImGui::Text("Walk into red sphere = hit!");
                ImGui::End();
            }

            // Window title
            if (currentTime - lastTime >= 1.0) {
                double fps = frameCount / (currentTime - lastTime);
                glfwSetWindowTitle(window,
                    ("Alpha + Physics Demo - FPS: " + std::to_string(static_cast<int>(fps))).c_str());
                frameCount = 0;
                lastTime   = currentTime;
            }

            // ============================================================
            // Draw helper — uploads per-object uniforms then draws all meshes
            // ============================================================
            auto draw_lit = [&](std::shared_ptr<Model>& mdl,
                                const Material& mat,
                                float alpha)
            {
                if (!mdl) return;
                for (auto const& pkg : mdl->meshes) {
                    pkg.shader->use();
                    pkg.shader->setUniform("uP_m", projection_matrix);
                    pkg.shader->setUniform("uV_m", view_matrix);

                    glm::mat4 localM = Model::createModelMatrix(
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
            };

            // Predefined materials
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
                glm::vec3(0.2f,  0.04f, 0.04f),
                glm::vec3(0.95f, 0.1f,  0.1f),
                glm::vec3(0.8f,  0.3f,  0.3f),
                32.0f
            };

            // ============================================================
            // Pass 1: Opaque objects
            // (depth write ON, depth test ON, blending OFF — default state)
            // ============================================================
            draw_lit(ground,         MAT_GROUND, 1.0f);
            draw_lit(model,          MAT_BUNNY,  1.0f);
            draw_lit(textured_model, MAT_SPHERE, 1.0f);
            draw_lit(enemy_model,    MAT_ENEMY,  1.0f);

            // ============================================================
            // Pass 2: Transparent objects — Task 1 (painter's algorithm)
            //
            // Rules:
            //  • Render AFTER all opaque objects so the depth buffer is full.
            //  • Sort back-to-front so closer glass composites over farther.
            //  • Disable depth writes so transparent surfaces don't occlude
            //    each other (depth TEST stays on so they're hidden by opaque).
            //  • Enable alpha blending: SRC_ALPHA / ONE_MINUS_SRC_ALPHA.
            // ============================================================
            {
                // Sort a local copy — don't modify the stored order
                auto sorted = transparent_objects;
                std::sort(sorted.begin(), sorted.end(),
                    [&](const GlassObject& a, const GlassObject& b) {
                        float da = glm::distance(camera.Position, a.model->getPosition());
                        float db = glm::distance(camera.Position, b.model->getPosition());
                        return da > db; // descending = back-to-front
                    });

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE); // keep reading depth, stop writing it

                for (auto& go : sorted)
                    draw_lit(go.model, go.mat, go.model->alpha);

                // ============================================================
                // Pass 3: Particle system — Task 3
                // Drawn inside the blended pass (same GL state: blend ON, depth write OFF)
                // ============================================================
                if (!particle_system.empty())
                    particle_system.draw(*shader_library.at("particle"),
                                        view_matrix, projection_matrix);

                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
            }

            if (FPS.is_updated())
                std::cout << "FPS: " << FPS.get() << "\n";

            if (show_imgui) {
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            glfwSwapBuffers(window);
            FPS.update();
        }
    }
    catch (std::exception const& e) {
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
    float ratio = static_cast<float>(width) / height;
    projection_matrix = glm::perspective(
        glm::radians(fov),
        ratio,
        0.1f,
        20000.0f
    );
}

void App::toggle_fullscreen(GLFWwindow* win) {
    if (isFullScreen) {
        glfwSetWindowMonitor(win, nullptr, savedXPos, savedYPos, savedWidth, savedHeight, GLFW_DONT_CARE);
        isFullScreen = false;
    } else {
        glfwGetWindowPos(win,  &savedXPos,  &savedYPos);
        glfwGetWindowSize(win, &savedWidth, &savedHeight);
        GLFWmonitor*       monitor = GetCurrentMonitor(win);
        const GLFWvidmode* mode    = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(win, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        isFullScreen = true;
    }
}

GLFWmonitor* App::GetCurrentMonitor(GLFWwindow* win) {
    int monitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    if (!monitors) return glfwGetPrimaryMonitor();

    int windowX, windowY, windowWidth, windowHeight;
    glfwGetWindowPos(win,  &windowX,     &windowY);
    glfwGetWindowSize(win, &windowWidth, &windowHeight);

    GLFWmonitor* bestMonitor = nullptr;
    int          bestOverlap = 0;

    for (int i = 0; i < monitorCount; i++) {
        GLFWmonitor* monitor = monitors[i];
        int mx, my;
        glfwGetMonitorPos(monitor, &mx, &my);
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode) continue;

        int ox = std::max(0, std::min(windowX + windowWidth,  mx + mode->width)  - std::max(windowX, mx));
        int oy = std::max(0, std::min(windowY + windowHeight, my + mode->height) - std::max(windowY, my));
        int area = ox * oy;
        if (area > bestOverlap) {
            bestOverlap  = area;
            bestMonitor = monitor;
        }
    }
    return bestMonitor ? bestMonitor : glfwGetPrimaryMonitor();
}

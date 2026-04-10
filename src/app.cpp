// app.cpp
#include <iostream>
#include <chrono>
#include <string>
#include <cmath>

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

    // Hidden until assets are ready
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

    // Capture cursor for free-fly camera
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Register callbacks
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

        camera.Position = glm::vec3(0.0f, 0.5f, 5.0f);

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
    //
    // Shaders
    //
    shader_library.emplace("simple_shader", std::make_shared<ShaderProgram>(
        std::filesystem::path("./basic.vert"),
        std::filesystem::path("./basic.frag")
    ));
    shader_library.emplace("rainbow", std::make_shared<ShaderProgram>(
        std::filesystem::path("./basic.vert"),
        std::filesystem::path("./GL_rainbow.frag")
    ));
    shader_library.emplace("texture_shader", std::make_shared<ShaderProgram>(
        std::filesystem::path("./tex.vert"),
        std::filesystem::path("./tex.frag")
    ));

    // Phong lighting shader (used by all scene models)
    shader_library.emplace("lighting", std::make_shared<ShaderProgram>(
        std::filesystem::path("./lighting.vert"),
        std::filesystem::path("./lighting.frag")
    ));

    //
    // Textures
    //
    texture_library.emplace("box",
        std::make_shared<Texture>(std::filesystem::path("../resources/textures/box.png")));
    texture_library.emplace("tex256",
        std::make_shared<Texture>(std::filesystem::path("../resources/textures/tex_256.png")));

    // Solid white — used for models with no UV texture so lighting is still visible
    texture_library.emplace("white",
        std::make_shared<Texture>(glm::vec3(1.0f, 1.0f, 1.0f)));

    //
    // Untextured model: bunny (lit with solid-white texture)
    //
    try {
        model = std::make_shared<Model>(
            std::filesystem::path("../obj_samples/bunny_tri_vn.obj"),
            shader_library.at("lighting")
        );
        model->setScale(glm::vec3(0.05f));
        model->setPosition(glm::vec3(0.175f, 0.16f, -1.05f));
        model->meshes[0].texture = texture_library.at("white");
        std::cout << "Loaded bunny\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Could not load bunny: " << e.what() << "\n";
    }

    //
    // Textured model: sphere (lit with box texture)
    //
    try {
        textured_model = std::make_shared<Model>(
            std::filesystem::path("../obj_samples/sphere_tri_vnt.obj"),
            shader_library.at("lighting")
        );
        textured_model->setScale(glm::vec3(1.5f));
        textured_model->setPosition(glm::vec3(3.5f, 0.0f, 0.0f));
        textured_model->meshes[0].texture = texture_library.at("box");
        std::cout << "Loaded textured sphere\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Could not load textured sphere: " << e.what() << "\n";
    }

    //
    // Lighting setup
    //

    // Task 1: Directional light (animated sun — starts at dusk angle)
    dirLight.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    dirLight.ambient   = glm::vec3(0.04f, 0.04f, 0.04f);
    dirLight.diffuse   = glm::vec3(1.0f,  0.9f,  0.7f);
    dirLight.specular  = glm::vec3(1.0f,  0.95f, 0.8f);

    // Task 2: Three point lights — different colors, orbits, speeds
    // Red-warm light: fast orbit at height +1.5
    pointLights[0].diffuse      = glm::vec3(1.0f, 0.25f, 0.1f);
    pointLights[0].specular     = glm::vec3(1.0f, 0.25f, 0.1f);
    pointLights[0].ambient      = glm::vec3(0.02f, 0.005f, 0.002f);
    pointLights[0].orbitRadius  = 4.0f;
    pointLights[0].orbitHeight  = 1.5f;
    pointLights[0].orbitSpeed   = 1.4f;
    pointLights[0].orbitAngle   = 0.0f;
    pointLights[0].linear       = 0.07f;
    pointLights[0].quadratic    = 0.017f;

    // Blue-cool light: medium orbit at height 0
    pointLights[1].diffuse      = glm::vec3(0.2f, 0.45f, 1.0f);
    pointLights[1].specular     = glm::vec3(0.2f, 0.45f, 1.0f);
    pointLights[1].ambient      = glm::vec3(0.004f, 0.009f, 0.02f);
    pointLights[1].orbitRadius  = 3.0f;
    pointLights[1].orbitHeight  = 0.0f;
    pointLights[1].orbitSpeed   = 0.8f;
    pointLights[1].orbitAngle   = 2.09f; // ~120 deg offset
    pointLights[1].linear       = 0.09f;
    pointLights[1].quadratic    = 0.032f;

    // Green light: slow wide orbit at height -0.5
    pointLights[2].diffuse      = glm::vec3(0.15f, 1.0f, 0.3f);
    pointLights[2].specular     = glm::vec3(0.15f, 1.0f, 0.3f);
    pointLights[2].ambient      = glm::vec3(0.003f, 0.02f, 0.006f);
    pointLights[2].orbitRadius  = 5.5f;
    pointLights[2].orbitHeight  = -0.5f;
    pointLights[2].orbitSpeed   = 0.5f;
    pointLights[2].orbitAngle   = 4.19f; // ~240 deg offset
    pointLights[2].linear       = 0.045f;
    pointLights[2].quadratic    = 0.0075f;

    // Task 3: Spotlight — camera headlight, starts on
    spotLight.direction   = glm::vec3(0.0f, 0.0f, -1.0f); // view space: -Z
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
                                const glm::mat4& view_matrix) const {
    glm::mat3 V3(view_matrix); // upper-left 3x3 for transforming directions

    // ---- Directional light (Task 1) ----
    glm::vec3 sunDirView = V3 * dirLight.direction;
    shader->setUniform("dirLight.direction", sunDirView);
    shader->setUniform("dirLight.ambient",   dirLight.ambient);
    shader->setUniform("dirLight.diffuse",   dirLight.diffuse);
    shader->setUniform("dirLight.specular",  dirLight.specular);

    // ---- Point lights (Task 2) ----
    for (int i = 0; i < 3; i++) {
        const std::string p = "pointLights[" + std::to_string(i) + "].";
        glm::vec3 posView = glm::vec3(view_matrix * glm::vec4(pointLights[i].currentWorldPos(), 1.0f));
        shader->setUniform(p + "position",  posView);
        shader->setUniform(p + "ambient",   pointLights[i].ambient);
        shader->setUniform(p + "diffuse",   pointLights[i].diffuse);
        shader->setUniform(p + "specular",  pointLights[i].specular);
        shader->setUniform(p + "constant",  pointLights[i].constant);
        shader->setUniform(p + "linear",    pointLights[i].linear);
        shader->setUniform(p + "quadratic", pointLights[i].quadratic);
    }

    // ---- Spotlight (Task 3) — always in view space: pos=origin, dir=(0,0,-1) ----
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
            frameCount++;

            // ---- Update lights ----

            // Task 1: Animate directional light (rotating sun arc)
            float sunAngle   = static_cast<float>(currentTime) * 0.25f;
            float elevation  = std::sin(sunAngle);          // -1..1, 1 = overhead
            float azimuth    = sunAngle;
            glm::vec3 sunDir = glm::normalize(glm::vec3(
                std::cos(azimuth),
                -std::abs(elevation) - 0.1f,  // always pointing somewhat downward
                std::sin(azimuth)
            ));
            dirLight.direction = sunDir;

            // Sun color: orange at horizon, cool-white at zenith
            float t = glm::clamp(std::abs(elevation), 0.0f, 1.0f);
            glm::vec3 sunColor = glm::mix(
                glm::vec3(1.0f, 0.4f, 0.1f),   // dawn/dusk: warm orange
                glm::vec3(1.0f, 0.97f, 0.88f),  // noon: cool white
                t
            );
            float sunBrightness = glm::clamp(std::abs(elevation) * 1.2f, 0.05f, 1.0f);
            dirLight.diffuse  = sunColor * sunBrightness * 0.8f;
            dirLight.specular = sunColor * sunBrightness * 0.9f;
            dirLight.ambient  = sunColor * 0.04f;

            // Task 2: Orbit point lights
            for (auto& pl : pointLights) {
                pl.orbitAngle += static_cast<float>(deltaTime) * pl.orbitSpeed;
            }

            // ---- Camera ----
            camera.Position += camera.ProcessInput(window, static_cast<GLfloat>(deltaTime));
            glm::mat4 view_matrix = camera.GetViewMatrix();

            // ---- Upload lighting uniforms to lighting shader (once per frame) ----
            auto& lighting_shader = shader_library.at("lighting");
            set_lighting_uniforms(lighting_shader, view_matrix);

            // ---- Clear ----
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ---- ImGui frame ----
            if (show_imgui) {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::Begin("Scene");
                ImGui::Text("FPS: %.1f",          ImGui::GetIO().Framerate);
                ImGui::Text("Camera: (%.1f, %.1f, %.1f)",
                    camera.Position.x, camera.Position.y, camera.Position.z);
                ImGui::Separator();

                ImGui::Text("Directional Light (sun)");
                float dir[3]{dirLight.direction.x, dirLight.direction.y, dirLight.direction.z};
                ImGui::SliderFloat3("Sun dir", dir, -1.0f, 1.0f);

                ImGui::Separator();
                ImGui::Text("Spotlight (H = toggle)");
                bool headlight = spotLight.on;
                if (ImGui::Checkbox("Headlight ON", &headlight))
                    spotLight.on = headlight;

                ImGui::End();
            }

            // ---- Window title ----
            if (currentTime - lastTime >= 1.0) {
                double fps = frameCount / (currentTime - lastTime);
                glfwSetWindowTitle(window,
                    ("Lighting Demo - FPS: " + std::to_string(static_cast<int>(fps))).c_str());
                frameCount = 0;
                lastTime   = currentTime;
            }

            // ---- Draw models with Phong lighting ----
            //
            // Material properties differ per model:
            //   matte bunny:  low specular, rougher surface
            //   shiny sphere: high specular, smooth surface
            //
            auto draw_lit = [&](std::shared_ptr<Model>& mdl,
                                glm::vec3 mat_ambient,
                                glm::vec3 mat_diffuse,
                                glm::vec3 mat_specular,
                                float     mat_shininess)
            {
                if (!mdl) return;
                for (auto const& pkg : mdl->meshes) {
                    pkg.shader->use();
                    pkg.shader->setUniform("uP_m", projection_matrix);
                    pkg.shader->setUniform("uV_m", view_matrix);

                    glm::mat4 mesh_local = Model::createModelMatrix(
                        pkg.origin, pkg.eulerAngles, pkg.scale);
                    pkg.shader->setUniform("uM_m", mdl->getModelMatrix() * mesh_local);

                    pkg.shader->setUniform("mat_ambient",   mat_ambient);
                    pkg.shader->setUniform("mat_diffuse",   mat_diffuse);
                    pkg.shader->setUniform("mat_specular",  mat_specular);
                    pkg.shader->setUniform("mat_shininess", mat_shininess);

                    if (pkg.texture) {
                        pkg.texture->bind(0);
                        pkg.shader->setUniform("tex0", 0);
                    }

                    pkg.mesh->draw();
                }
            };

            // Bunny: matte, chalky white
            draw_lit(model,
                glm::vec3(0.2f),
                glm::vec3(0.8f),
                glm::vec3(0.15f),
                16.0f);

            // Sphere: shiny, metallic feel
            draw_lit(textured_model,
                glm::vec3(0.15f),
                glm::vec3(0.9f),
                glm::vec3(0.7f),
                64.0f);

            if (FPS.is_updated())
                std::cout << "FPS: " << FPS.get() << "\n";

            // ---- Finalize ImGui ----
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
        glfwGetWindowPos(win,  &savedXPos,   &savedYPos);
        glfwGetWindowSize(win, &savedWidth,  &savedHeight);
        GLFWmonitor*      monitor = GetCurrentMonitor(win);
        const GLFWvidmode* mode  = glfwGetVideoMode(monitor);
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

    GLFWmonitor* bestMonitor  = nullptr;
    int          bestOverlap  = 0;

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

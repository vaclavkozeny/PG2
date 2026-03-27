// app.cpp
#include <iostream>
#include <chrono>
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
#include "gl_err_callback.h"
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
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

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

    //
    // Textures
    //
    texture_library.emplace("box",
        std::make_shared<Texture>(std::filesystem::path("../resources/textures/box.png")));
    texture_library.emplace("tex256",
        std::make_shared<Texture>(std::filesystem::path("../resources/textures/tex_256.png")));

    //
    // Untextured model: bunny
    //
    try {
        model = std::make_shared<Model>(
            std::filesystem::path("../obj_samples/bunny_tri_vn.obj"),
            shader_library.at("simple_shader")
        );
        model->setScale(glm::vec3(0.05f));
        model->setPosition(glm::vec3(0.175f, 0.16f, -1.05f));
        std::cout << "Loaded bunny\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Could not load bunny: " << e.what() << "\n";
    }

    //
    // Textured model: sphere
    //
    try {
        textured_model = std::make_shared<Model>(
            std::filesystem::path("../obj_samples/sphere_tri_vnt.obj"),
            shader_library.at("texture_shader")
        );
        textured_model->setScale(glm::vec3(1.5f));
        textured_model->setPosition(glm::vec3(3.5f, 0.0f, 0.0f));
        textured_model->meshes[0].texture = texture_library.at("box");
        std::cout << "Loaded textured sphere\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Could not load textured sphere: " << e.what() << "\n";
    }

    std::cout << "Assets initialized...\n";
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

            // ImGui frame
            if (show_imgui) {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::Begin("Performance");
                ImGui::Text("FPS: %.1f",          ImGui::GetIO().Framerate);
                ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
                ImGui::Text("Camera: (%.1f, %.1f, %.1f)",
                    camera.Position.x, camera.Position.y, camera.Position.z);
                ImGui::End();
            }

            // Window title update (once per second)
            if (currentTime - lastTime >= 1.0) {
                double fps = frameCount / (currentTime - lastTime);
                glfwSetWindowTitle(window,
                    ("OpenGL context - FPS: " + std::to_string(static_cast<int>(fps))).c_str());
                frameCount = 0;
                lastTime   = currentTime;
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Camera
            camera.Position += camera.ProcessInput(window, static_cast<GLfloat>(deltaTime));
            glm::mat4 view_matrix = camera.GetViewMatrix();

            // Draw helper — sets MVP, binds texture if present, then draws
            auto draw_model = [&](std::shared_ptr<Model>& mdl) {
                if (!mdl) return;
                for (auto const& pkg : mdl->meshes) {
                    pkg.shader->use();
                    pkg.shader->setUniform("uP_m", projection_matrix);
                    pkg.shader->setUniform("uV_m", view_matrix);

                    glm::mat4 mesh_local = Model::createModelMatrix(
                        pkg.origin, pkg.eulerAngles, pkg.scale);
                    pkg.shader->setUniform("uM_m", mdl->getModelMatrix() * mesh_local);

                    if (pkg.texture) {
                        pkg.texture->bind(0);
                        pkg.shader->setUniform("tex0", 0);
                    }

                    pkg.mesh->draw();
                }
            };

            draw_model(model);
            draw_model(textured_model);

            if (FPS.is_updated())
                std::cout << "FPS: " << FPS.get() << "\n";

            // Finalize ImGui
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

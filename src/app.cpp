

#include <iostream>
#include <format>
#include <chrono>
#include <stack>
#include <random>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <array>

#include <opencv2/opencv.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "app.hpp"
#include "assets.hpp"
#include "getinfo.hpp"
#include "gl_err_callback.h"
#include "lights.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

// ImGUI headers
#include <imgui.h>               // main ImGUI header
#include <imgui_impl_glfw.h>     // GLFW bindings
#include <imgui_impl_opengl3.h>  // OpenGL bindings

#include <fps_meter.hpp>
#include <algorithm>

using json = nlohmann::json;

#include "meshgen.hpp"

App::App()
{
    // default constructor
    // nothing to do here (so far...)
    std::cout << "Constructed...\n";
}
// ========================================================================= //
//                               INITIALIZATION                              //
// ========================================================================= //
bool App::init()
{
    try {
        // GL init
        init_gl();
        init_assets();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);
        init_imgui();
        glfwShowWindow(window);
    }
    catch (std::exception const& e) {
        std::cerr << "Init failed : " << e.what() << std::endl;
        throw;
    }
    std::cout << "Initialized...\n";
    return true;
}
// ========================================================================= //
//                               INITIALIZATION                              //
// ========================================================================= //
void error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}
void App::init_assets(void) {
    shader_library.emplace("simple_shader", std::make_shared<shader_program>((std::filesystem::path)"shaders/basic_core.vert",(std::filesystem::path)"shaders/basic.frag"));
    shader_library.emplace("rainbow", std::make_shared<shader_program>((std::filesystem::path)"shaders/basic_core.vert",(std::filesystem::path)"shaders/GL_rainbow.frag"));
    shader_library.emplace("texture", std::make_shared<shader_program>((std::filesystem::path)"shaders/tex.vert",(std::filesystem::path)"shaders/tex.frag"));

    shader_library.emplace("lighting", std::make_shared<shader_program>((std::filesystem::path)"shaders/lighting.vert",(std::filesystem::path)"shaders/lighting.frag"));


    texture_library.emplace("wood_box", std::make_shared<Texture>((std::filesystem::path)"resources/textures/box_rgb888.png"));
    texture_library.emplace("mix", std::make_shared<Texture>((std::filesystem::path)"resources/textures/tex_1024.jpg"));

    //mesh_library.emplace("cube", generateCube());
    //mesh_library.emplace("sphere", generateSphere(36, 18));

    Model myModel((std::filesystem::path)"resources/models/teapot.obj", shader_library.at("lighting"), texture_library.at("wood_box"));
    myModel.setScale(glm::vec3(0.3f)); // scale down the teapot
    scene.emplace("teapot_object", myModel);

    const float floor_y = 0.0f;
    const float floor_half_size = 40.0f;
    std::vector<vertex> floor_vertices{
        {{-floor_half_size, floor_y, -floor_half_size}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ floor_half_size, floor_y,  floor_half_size}, {0.0f, 1.0f, 0.0f}, {40.0f, 40.0f}},
        {{ floor_half_size, floor_y, -floor_half_size}, {0.0f, 1.0f, 0.0f}, {40.0f, 0.0f}},
        {{-floor_half_size, floor_y, -floor_half_size}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-floor_half_size, floor_y,  floor_half_size}, {0.0f, 1.0f, 0.0f}, {0.0f, 40.0f}},
        {{ floor_half_size, floor_y,  floor_half_size}, {0.0f, 1.0f, 0.0f}, {40.0f, 40.0f}},
    };
    auto floor_mesh = std::make_shared<Mesh>(floor_vertices, GL_TRIANGLES);
    Model floor_model;
    floor_model.addMesh(floor_mesh, shader_library.at("lighting"), texture_library.at("wood_box"));
    scene.emplace("floor_object", floor_model);

    Model bunny_model((std::filesystem::path)"resources/models/bunny.obj", shader_library.at("lighting"), texture_library.at("mix"));
    bunny_model.setPosition(glm::vec3(8.0f, 0.0f, 0.0f));
    bunny_model.is_transparent = true;
    scene.emplace("bunny_object", bunny_model);
}
void App::init_gl(void){
    load_config("resources/config.json");
    //std::cout << config.title << "\n";
    // init glfw
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        throw std::runtime_error("GLWF not ok! test");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

	// open window, but hidden - it will be enabled later, after asset initialization
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);


    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // open window (GL canvas) with no special properties
    window = glfwCreateWindow(config.width, config.height, config.title.c_str(), NULL, NULL);
    if (!window)
        throw std::runtime_error("GLFW window init failed");
    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &width, &height);
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, fbsize_callback);    // On GL framebuffer resize callback.
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSwapInterval(vsync_enabled ? 1 : 0); // vsync on/off
    // init glew
    auto glew = glewInit();
    if (glew != GLEW_OK)
        throw std::runtime_error(std::format("GLEW not ok! Error: {0}",reinterpret_cast<const char*>(glewGetErrorString(glew))));

    if (!GLEW_ARB_direct_state_access)
        throw std::runtime_error("No DSA :-(");
    if (GLEW_ARB_debug_output)
    {
        glDebugMessageCallback(MessageCallback, 0);
        glEnable(GL_DEBUG_OUTPUT);

        //default is asynchronous debug output, use this to simulate glGetError() functionality
        //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        std::cout << "GL_DEBUG enabled." << std::endl;
    }
    else
        std::cout << "GL_DEBUG NOT SUPPORTED!" << std::endl;

    auto h = getInfo();
    std::cout << h.vendor << "\n" << h.renderer << "\n" << h.version << "\n" << h.glsl << std::endl;
    std::cout << getProfile() << std::endl;
}
void App::init_imgui()
{
	// see https://github.com/ocornut/imgui/wiki/Getting-Started

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
	std::cout << "ImGUI version: " << ImGui::GetVersion() << "\n";
}

// ========================================================================= //
//                                 MAIN LOOP                                 //
// ========================================================================= //
int App::run(void)
{
    update_projection_matrix();
    glViewport(0, 0, width, height);
    // Activate shader program. There is only one program, so activation can be out of the loop.
    // In more realistic scenarios, you will activate different shaders for different 3D objects.
    glUseProgram(shader_prog_ID);
    glEnable(GL_DEPTH_TEST);
    if (msaa_enabled) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }
    glEnable(GL_CULL_FACE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // get first position of mouse cursor
    glfwGetCursorPos(window, &cursorLastX, &cursorLastY);
    fps_meter FPS;
    const glm::vec3 teapot_center{0.05425f, 0.39375f, 0.0f};
    camera.Position = teapot_center + glm::vec3(0.0f, 3.0f, 12.0f);
    camera.LookAt(teapot_center);
    double last_frame_time = glfwGetTime();

    // Setup lighting
    LightingSetup lighting_setup;
    lighting_setup.use_directional_light = true;   // Task 1: Use directional light (sun)
    lighting_setup.use_point_lights = false;
    lighting_setup.num_point_lights = 0;

    // Configure directional light (sun)
    lighting_setup.directional_light = DirectionalLight(
        glm::vec3(0.0f, -1.0f, 0.0f),  // Initial direction (straight down)
        glm::vec3(1.0f, 1.0f, 1.0f),   // Initial color (white)
        1.0f                            // Intensity
    );

    // Configure material properties
    lighting_setup.material = MaterialProperties(
        glm::vec3(1.0f),  // ambient
        glm::vec3(1.0f),  // diffuse
        glm::vec3(1.0f),  // specular
        32.0f             // shininess
    );

    // Configure light intensities
    lighting_setup.light_intensity = LightIntensity(
        glm::vec3(0.6f),  // ambient intensity
        glm::vec3(0.6f),  // diffuse intensity
        glm::vec3(0.35f)  // specular intensity
    );

    try {
        while (!glfwWindowShouldClose(window)) {
            GLfloat t = glfwGetTime();
            //auto current_shader = shader_library.at("texture"); // crated a copy of shared pointer. Shader is guaranteed to live.
            //current_shader->use();

            // Process camera input BEFORE ImGui so it always works
            double now = glfwGetTime();
            double delta_t = now - last_frame_time;
            last_frame_time = now;
            camera.Position += camera.ProcessInput(window, delta_t); // process keys etc.

            if (show_imgui) {
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();
				// Don't let ImGui capture keyboard so camera controls always work
				//ImGui::GetIO().WantCaptureKeyboard = false;
				ImGui::SetNextWindowPos(ImVec2(10, 10));
				ImGui::SetNextWindowSize(ImVec2(250, 100));

				ImGui::Begin("Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
				ImGui::Text("V-Sync: %s", vsync_enabled ? "ON" : "OFF");
                ImGui::Text("MSAA: %s", msaa_enabled ? "ON" : "OFF");
				ImGui::Text("FPS: %.1f", FPS.get());
				ImGui::Text("(press RMB to release mouse)");
                ImGui::Text("(hit G to show/hide info, M to toggle MSAA) tetst");
				ImGui::End();
			}
            //current_shader->setUniform("iTime", t);
            // clear canvas
            glClearColor(bg_r,bg_g,bg_b,0.5f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // set uniforms for shader
            auto lighting = shader_library.at("lighting");

            lighting->setUniform("uV_m", camera.GetViewMatrix());
            lighting->setUniform("uP_m", projection_matrix);

            // Animate directional light (sun) - Task 1
            // Sun moves across the sky (simulate time of day)
            float sun_time = fmod(t, 20.0f) / 20.0f;  // Cycle every 20 seconds
            float sun_angle_horizontal = sun_time * 2.0f * glm::pi<float>(); // Full circle
            float sun_angle_vertical = glm::sin(sun_time * glm::pi<float>()) * glm::pi<float>() * 0.4f; // Varies from -0.4π to +0.4π

            // Calculate sun direction in world space
            glm::vec3 sun_direction(
                std::sin(sun_angle_horizontal) * std::cos(sun_angle_vertical),
                -std::cos(sun_angle_vertical),  // Negative because light direction points "down"
                std::cos(sun_angle_horizontal) * std::cos(sun_angle_vertical)
            );

            lighting_setup.directional_light.direction = sun_direction;

            // Transform light direction to view space
            glm::vec3 light_direction_view = glm::normalize(glm::vec3(
                camera.GetViewMatrix() * glm::vec4(lighting_setup.directional_light.direction, 0.0f)
            ));

            // Set lighting uniforms
            lighting->setUniform("use_directional_light", lighting_setup.use_directional_light);
            lighting->setUniform("use_point_lights", lighting_setup.use_point_lights);
            lighting->setUniform("num_point_lights", lighting_setup.num_point_lights);

            if (lighting_setup.use_directional_light) {
                lighting->setUniform("light_direction", light_direction_view);
            }

            // Set material and light intensity uniforms
            lighting->setUniform("ambient_intensity", lighting_setup.light_intensity.ambient);
            lighting->setUniform("diffuse_intensity", lighting_setup.light_intensity.diffuse);
            lighting->setUniform("specular_intensity", lighting_setup.light_intensity.specular);
            lighting->setUniform("specular_shinines", lighting_setup.material.shininess);
            lighting->setUniform("ambient_material", lighting_setup.material.ambient);
            lighting->setUniform("diffuse_material", lighting_setup.material.diffuse);
            lighting->setUniform("specular_material", lighting_setup.material.specular);

            std::vector<Model*> transparent_models;
            transparent_models.reserve(scene.size());

            // Opaque pass + collect transparent objects
            lighting->setUniform("uAlpha", 1.0f);
            for (auto& [name, model] : scene) {
                if (model.is_transparent) {
                    transparent_models.emplace_back(&model);
                    continue;
                }
                model.draw();
            }

            // Transparent pass (back-to-front)
            std::sort(transparent_models.begin(), transparent_models.end(), [&](Model* a, Model* b) {
                return glm::distance(camera.Position, a->getPosition()) > glm::distance(camera.Position, b->getPosition());
            });

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            lighting->setUniform("uAlpha", 0.7f);
            for (auto* model : transparent_models) {
                model->draw();
            }
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            // poll events, call callbacks, flip back<->front buffer
            glfwPollEvents();
            if (show_imgui) {
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			}
            glfwSwapBuffers(window);
            FPS.update();
        }
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Finished OK...\n";
    return EXIT_SUCCESS;
}

// ========================================================================= //
//                                 CALLBACKS                                 //
// ========================================================================= //
void App::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    App* app = (App*)(glfwGetWindowUserPointer(window));
    if(action == GLFW_PRESS){
        switch (key)
        {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_V:
            app->vsync_enabled = !app->vsync_enabled;
            glfwSwapInterval(app->vsync_enabled ? 1 : 0);
            break;
        case GLFW_KEY_G:
            app->show_imgui = !app->show_imgui;
            break;
        case GLFW_KEY_M:
            app->msaa_enabled = !app->msaa_enabled;
            if (app->msaa_enabled) {
                glEnable(GL_MULTISAMPLE);
            } else {
                glDisable(GL_MULTISAMPLE);
            }
            break;
        case GLFW_KEY_F11:
            app->toggle_fullscreen(window);
            break;
        case GLFW_KEY_PRINT_SCREEN:
            app->save_screenshot();
            break;
        default:
            break;
        }
    }
}
void App::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    App* app = (App*)(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
		switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT: {
			int mode = glfwGetInputMode(window, GLFW_CURSOR);
			if (mode == GLFW_CURSOR_NORMAL) {
				// we are outside of application, catch the cursor
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                glfwGetCursorPos(window, &app->cursorLastX, &app->cursorLastY);
			}
			else {
				std::cout << "Bang!\n";
			}
			break;
		}
		case GLFW_MOUSE_BUTTON_RIGHT:
            // release the cursor
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			break;
		default:
			break;
		}
	}
}
void App::scroll_callback(GLFWwindow * window, double xoffset, double yoffset)
{
    // get App instance
    App* app = (App*)(glfwGetWindowUserPointer(window));
    app->fov += 10*yoffset; // yoffset is mostly +1 or -1; one degree difference in fov is not visible
    app->fov = std::clamp(app->fov, 20.0f, 170.0f); // limit FOV to reasonable values...

    app->update_projection_matrix();
}
void App::fbsize_callback(GLFWwindow* window, int width, int height)
{
    App* app = (App*)(glfwGetWindowUserPointer(window));
    app->width = width;
    app->height = height;

    // set viewport
    glViewport(0, 0, width, height);
    //now your canvas has [0,0] in bottom left corner, and its size is [width x height]

    app->update_projection_matrix();
}
void App::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    auto app = static_cast<App*>(glfwGetWindowUserPointer(window));

    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
        app->cursorLastX = xpos;
        app->cursorLastY = ypos;
        return;
    }

    app->camera.ProcessMouseMovement(xpos - app->cursorLastX, (ypos - app->cursorLastY) * -1.0);
    app->cursorLastX = xpos;
    app->cursorLastY = ypos;
}
// ========================================================================= //
//                              HELPER FUNCTIONS                             //
// ========================================================================= //
void App::load_config(std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open config file, using defaults.\n";
        return;
    }

    try {
        json j;
        file >> j;

        config.width = j["window"].value("width", 800);
        config.height = j["window"].value("height", 600);
        config.title = j["window"].value("title", "OpenGL Window");
        config.vsync = j["window"].value("vsync", false);
        config.msaa = j["window"].value("msaa", false);
        vsync_enabled = config.vsync;
        msaa_enabled = config.msaa;
    }
    catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
    }
}
GLFWmonitor* App::GetCurrentMonitor(GLFWwindow* window) {
    // get all monitors
    int monitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    if (!monitors) return glfwGetPrimaryMonitor();

    //store window params
    int windowX, windowY, windowWidth, windowHeight;
    glfwGetWindowPos(window, &windowX, &windowY);
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    GLFWmonitor* bestMonitor = nullptr;
    int bestOverlapArea = 0;

    //find best overlap
    for (int i = 0; i < monitorCount; i++) {
        GLFWmonitor* monitor = monitors[i];

        int monitorX, monitorY;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode) continue;

        // Calculate the intersection (overlap) between the window and the monitor
        int overlapX = std::max(0, std::min(windowX + windowWidth, monitorX + mode->width) - std::max(windowX, monitorX));
        int overlapY = std::max(0, std::min(windowY + windowHeight, monitorY + mode->height) - std::max(windowY, monitorY));
        int overlapArea = overlapX * overlapY;

        if (overlapArea > bestOverlapArea) {
            bestOverlapArea = overlapArea;
            bestMonitor = monitor;
        }
    }
    return bestMonitor ? bestMonitor : glfwGetPrimaryMonitor();
}
void App::toggle_fullscreen(GLFWwindow* window) {
    if (isFullScreen) {
        // --- RESTORE TO WINDOWED MODE ---
        glfwSetWindowMonitor(window, nullptr, savedXPos, savedYPos, savedWidth, savedHeight, GLFW_DONT_CARE);
        isFullScreen = false;
    } else {
        // --- SWITCH TO FULLSCREEN MODE ---
        // 1. Save current window position and size
        glfwGetWindowPos(window, &savedXPos, &savedYPos);
        glfwGetWindowSize(window, &savedWidth, &savedHeight);

        // 2. Determine which monitor the window is currently on
        GLFWmonitor* monitor = GetCurrentMonitor(window);
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // 3. Switch to fullscreen on that specific monitor
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        isFullScreen = true;
    }
}
void App::update_projection_matrix(void)
{
    if (height < 1)
        height = 1;   // avoid division by 0

    float ratio = static_cast<float>(width) / height;

    projection_matrix = glm::perspective(
        glm::radians(fov),   // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90� (extra wide) and 30� (quite zoomed in)
        ratio,               // Aspect Ratio. Depends on the size of your window.
        0.1f,                // Near clipping plane. Keep as big as possible, or you'll get precision issues.
        20000.0f             // Far clipping plane. Keep as little as possible.
    );
}
void App::save_screenshot(void)
{
    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    if (fbWidth <= 0 || fbHeight <= 0) {
        std::cerr << "Screenshot failed: invalid framebuffer size.\n";
        return;
    }

    std::vector<unsigned char> pixels(fbWidth * fbHeight * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, fbWidth, fbHeight, GL_BGR, GL_UNSIGNED_BYTE, pixels.data());

    cv::Mat image(fbHeight, fbWidth, CV_8UC3, pixels.data());
    cv::Mat flipped;
    cv::flip(image, flipped, 0);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    std::ostringstream name;
    name << "screenshot_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".png";

    if (!cv::imwrite(name.str(), flipped)) {
        std::cerr << "Screenshot failed: could not write " << name.str() << "\n";
        return;
    }

    std::cout << "Saved screenshot: " << name.str() << std::endl;
}
// ========================================================================= //
//                                  CLEANUP                                  //
// ========================================================================= //
void App::destroy(void)
{
    //buffer cleanup
    glDeleteProgram(shader_prog_ID);
    glDeleteBuffers(1, &VBO_ID);
    glDeleteVertexArrays(1, &VAO_ID);
	// clean up ImGUI
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// clean up OpenCV
	cv::destroyAllWindows();

	// clean-up GLFW
	if (window) {
		glfwDestroyWindow(window);
		window = nullptr;
	}
	glfwTerminate();
}

App::~App()
{
    destroy();
    std::cout << "Bye...\n";
}

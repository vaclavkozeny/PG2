

#include <iostream>
#include <format>
#include <chrono>
#include <stack>
#include <random>

#include <opencv2/opencv.hpp>
#include <GL/glew.h> 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "app.hpp"
#include "assets.hpp"
#include "getinfo.hpp"
#include "gl_err_callback.h"

#include <fstream>
#include <nlohmann/json.hpp>

// ImGUI headers
#include <imgui.h>               // main ImGUI header
#include <imgui_impl_glfw.h>     // GLFW bindings
#include <imgui_impl_opengl3.h>  // OpenGL bindings

#include <fps_meter.hpp>
#include <algorithm>

using json = nlohmann::json;


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
    //
    // Initialize pipeline: compile, link and use shaders
    //
    
    //SHADERS - define & compile & link
    const char* vertex_shader =
        "#version 460 core\n"
        "in vec3 attribute_Position;"
        "void main() {"
        "  gl_Position = vec4(attribute_Position, 1.0);"
        "}";

    const char* fragment_shader =
        "#version 460 core\n"
        "uniform vec4 uniform_Color;"
        "out vec4 FragColor;"
        "void main() {"
        "  FragColor = uniform_Color;"
        "}";
    
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);
    
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);
    
    shader_prog_ID = glCreateProgram();
    glAttachShader(shader_prog_ID, fs);
    glAttachShader(shader_prog_ID, vs);
    glLinkProgram(shader_prog_ID);
    
    //now we can delete shader parts (they can be reused, if you have more shaders)
    //the final shader program already linked and stored separately
    glDetachShader(shader_prog_ID, fs);
    glDetachShader(shader_prog_ID, vs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // 
    // Create and load data into GPU using OpenGL DSA (Direct State Access)
    //
    
    // Create VAO + data description (similar to container)
    glCreateVertexArrays(1, &VAO_ID);

    GLint position_attrib_location = glGetAttribLocation(shader_prog_ID, "attribute_Position");

    glEnableVertexArrayAttrib(VAO_ID, position_attrib_location);
    glVertexArrayAttribFormat(VAO_ID, position_attrib_location, glm::vec3::length(), GL_FLOAT, GL_FALSE, offsetof(vertex, position));
    glVertexArrayAttribBinding(VAO_ID, position_attrib_location, 0); // (GLuint vaobj, GLuint attribindex, GLuint bindingindex)

    // Create and fill data
    glCreateBuffers(1, &VBO_ID);
    glNamedBufferData(VBO_ID, triangle_vertices.size()*sizeof(vertex), triangle_vertices.data(), GL_STATIC_DRAW);

    // Connect together
    glVertexArrayVertexBuffer(VAO_ID, 0, VBO_ID, 0, sizeof(vertex)); // (GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)
}
void App::init_gl(void){
    load_config("config.json");
    //std::cout << config.title << "\n";
    // init glfw
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        throw std::runtime_error("GLWF not ok!");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// open window, but hidden - it will be enabled later, after asset initialization
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);


    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // open window (GL canvas) with no special properties
    window = glfwCreateWindow(800, 600, "OpenGL context", NULL, NULL);
    if (!window)
        throw std::runtime_error("GLFW window init failed");
    glfwMakeContextCurrent(window);
    
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSwapInterval(0);
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
    GLfloat r,g,b,a;
    r=g=b=a=1.0f; //white color

    // Activate shader program. There is only one program, so activation can be out of the loop. 
    // In more realistic scenarios, you will activate different shaders for different 3D objects.
    glUseProgram(shader_prog_ID);

    // Get uniform location in GPU program. This will not change, so it can be moved out of the game loop.
    GLint uniform_color_location = glGetUniformLocation(shader_prog_ID, "uniform_Color");
    if (uniform_color_location == -1) {
        std::cerr << "Uniform location is not found in active shader program. Did you forget to activate it?\n";
    }
    
    fps_meter FPS;
    try {
        while (!glfwWindowShouldClose(window)) {
            double t = glfwGetTime();
            if (show_imgui) {
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();
				//ImGui::ShowDemoWindow(); // Enable mouse when using Demo!
				ImGui::SetNextWindowPos(ImVec2(10, 10));
				ImGui::SetNextWindowSize(ImVec2(250, 100));

				ImGui::Begin("Info", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
				ImGui::Text("V-Sync: %s", vsync_enabled ? "ON" : "OFF");
				ImGui::Text("FPS: %.1f", FPS.get());
				ImGui::Text("(press RMB to release mouse)");
				ImGui::Text("(hit G to show/hide info)");
				ImGui::End();
			}
            triangle_r = (float)(sin(t * 1.0f) * 0.5 + 0.5); 
            triangle_g = (float)(sin(t * 1.3f) * 0.5 + 0.5); 
            triangle_b = (float)(sin(t * 1.7f) * 0.5 + 0.5);
            // clear canvas
            glClearColor(bg_r,bg_g,bg_b,0.5f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //set uniform parameter for shader
            // (try to change the color in key callback)          
            glUniform4f(uniform_color_location, triangle_r, triangle_g, triangle_b, a);
            
            //bind 3d object data
            glBindVertexArray(VAO_ID);

            // draw all VAO data
            glDrawArrays(GL_TRIANGLES, 0, triangle_vertices.size());

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
        case GLFW_KEY_F11:
            app->toggle_fullscreen(window);
            break;
        default:
            break;
        }
    }
}
void App::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    //App* app = (App*)glfwGetWindowUserPointer(window);
    if (action == GLFW_PRESS) {
		switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT: {
			int mode = glfwGetInputMode(window, GLFW_CURSOR);
			if (mode == GLFW_CURSOR_NORMAL) {
				// we are outside of application, catch the cursor
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
void App::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    App* app = (App*)glfwGetWindowUserPointer(window);
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
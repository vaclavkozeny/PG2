// icp.cpp 
// author: JJ

// C++
// include anywhere, in any order
#include <iostream>
#include <chrono>
#include <stack>
#include <random>

// OpenCV (does not depend on GL)
#include <opencv4/opencv2/opencv.hpp>

// OpenGL Extension Wrangler: allow all multiplatform GL functions
#include <GL/glew.h> 
// WGLEW = Windows GL Extension Wrangler (change for different platform) 
// platform specific functions (in this case Windows)
// #include <GL/wglew.h> 

// GLFW toolkit
// Uses GL calls to open GL context, i.e. GLEW __MUST__ be first.
#include <GLFW/glfw3.h>

// OpenGL math (and other additional GL libraries, at the end)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "assets.hpp"

// ImGUI headers
#include <imgui.h>               // main ImGUI header
#include <imgui_impl_glfw.h>     // GLFW bindings
#include <imgui_impl_opengl3.h>  // OpenGL bindings

#include "fps_meter.hpp"

//---------------------------------------------------------------------
#include "app.hpp"

//#include "glerror.h"
#include "gl_err_callback.h"

App::App()
{
    // default constructor
    // nothing to do here (so far...)
    std::cout << "Constructed...\n";
}

void App::initOpenGL() {
    /* Initialize the library */
	glfwSetErrorCallback(glfw_error_callback);

	if (!glfwInit()) {
		throw std::runtime_error("GLFW can not be initialized.");
	}

	// open window, but hidden - it will be enabled later, after asset initialization
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    // Na Macu musíme explicitně říct, že chceme Core Profile 4.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(800, 600, "OpenGL context", NULL, NULL);
    if (!window) throw std::runtime_error("Window creation failed");
    
    glfwMakeContextCurrent(window);

    glfwSetWindowUserPointer(window, this);

    // disable mouse cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLFW callbacks registration
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
	glfwSetKeyCallback(window, glfw_key_callback);
	glfwSetScrollCallback(window, glfw_scroll_callback);

    glewExperimental = GL_TRUE;
    glewInit();
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

bool App::init()
{
    try {
        // GL init
        initOpenGL();
            
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

        if (vsync)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);
        
        glfwSwapInterval((vsync?1:0));

        //if (!std::filesystem::exists("resources"))
		//	throw std::runtime_error("Directory 'resources' not found. Various media files are expected to be there.");
		
		init_glfw();

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

void App::init_assets(void) {
    //
    // Initialize pipeline: compile, link and use shaders
    //
    
    // SHADERS - Upraveno na verzi 410 pro macOS (Apple Silicon)
    const char* vertex_shader =
        "#version 410 core\n"
        "layout(location = 0) in vec3 attribute_Position;"
        "void main() {"
        "  gl_Position = vec4(attribute_Position, 1.0);"
        "}";

    const char* fragment_shader =
        "#version 410 core\n"
        "uniform vec4 uniform_Color;"
        "out vec4 FragColor;"
        "void main() {"
        "  FragColor = uniform_Color;"
        "}";
    
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);
    
    // TODO: Zde by bylo dobré zkontrolovat chyby kompilace (glGetShaderiv)

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);
    
    shader_prog_ID = glCreateProgram();
    glAttachShader(shader_prog_ID, fs);
    glAttachShader(shader_prog_ID, vs);
    glLinkProgram(shader_prog_ID);
    
    glDetachShader(shader_prog_ID, fs);
    glDetachShader(shader_prog_ID, vs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // 
    // Create and load data into GPU - "Old-school" Bind-to-Edit styl (Mac kompatibilní)
    //
    
    // 1. VAO - Kontejner pro nastavení atributů
    glGenVertexArrays(1, &VAO_ID);
    glBindVertexArray(VAO_ID);

    // 2. VBO - Samotná data (souřadnice bodů)
    glGenBuffers(1, &VBO_ID);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
    glBufferData(GL_ARRAY_BUFFER, triangle_vertices.size() * sizeof(vertex), triangle_vertices.data(), GL_STATIC_DRAW);

    // 3. Propojení dat z VBO do Shaderu
    // Hledáme, kde v shaderu je proměnná "attribute_Position"
    GLint position_attrib_location = glGetAttribLocation(shader_prog_ID, "attribute_Position");
    
    if (position_attrib_location != -1) {
        glEnableVertexAttribArray(position_attrib_location);
        glVertexAttribPointer(
            position_attrib_location, 
            3,                          // x, y, z
            GL_FLOAT, 
            GL_FALSE, 
            sizeof(vertex),             // velikost jedné struktury vertex
            (void*)offsetof(vertex, position) // posun pozice v rámci struktury
        );
    } else {
        std::cerr << "Chyba: Atribut 'attribute_Position' nebyl v shaderu nalezen!\n";
    }

    // 4. Cleanup bindů - aby se další operace omylem nepropsaly sem
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Všechna volání glCreateVertexArrays, glNamedBufferData atd. musí pryč!
    std::cout << "Assets initialized (Non-DSA mode for Mac)...\n";
}

int App::run(void)
{
    try {
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

        lastTime = glfwGetTime();
        frameCount = 0;
        fps_meter FPS;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            double currentTime = glfwGetTime();
            frameCount++;

            // 2. Start ImGui Frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // 3. Define the UI
            {
                ImGui::Begin("Performance");
                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
                ImGui::End();
            }

            // update every second
            if (currentTime - lastTime >= 1.0)
            {
                double fps = frameCount / (currentTime - lastTime);

                std::string title = "OpenGL context - FPS: " + std::to_string((int)fps);
                glfwSetWindowTitle(window, title.c_str());

                frameCount = 0;
                lastTime = currentTime;
            }
            // clear canvas
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //set uniform parameter for shader
            // (try to change the color in key callback)          
            glUniform4f(uniform_color_location, triangle_r, g, b, a);
            
            //bind 3d object data
            glBindVertexArray(VAO_ID);

            // draw all VAO data
            glDrawArrays(GL_TRIANGLES, 0, triangle_vertices.size());

            // poll events, call callbacks, flip back<->front buffer
            //glfwPollEvents();
            //glfwSwapBuffers(window);    

            if (FPS.is_updated()) // display new value only once per interval (default = 1.0s)
			std::cout << "FPS: " << FPS.get() << std::endl;
            
            // 5. Finalize and Render ImGui
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // 6. Swap Buffers
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

// MOJE CALLBACKY
/*
void App::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));

    app->triangle_r += yoffset * 0.05f;

    if (app->triangle_r > 1.0f) app->triangle_r = 1.0f;
    if (app->triangle_r < 0.0f) app->triangle_r = 0.0f;
}

void App::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			std::cout << "ESC has been pressed!\n";
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
        case GLFW_KEY_V:
            app->vsync = !app->vsync;
            glfwSwapInterval(app->vsync ? 1 : 0);

            std::cout << "VSync: "
                      << (app->vsync ? "ON" : "OFF")
                      << std::endl;
        default:
            break;
        }
    }
}
*/

App::~App()
{
    //new stuff: cleanup GL data
    if (shader_prog_ID) glDeleteProgram(shader_prog_ID);
    if (VBO_ID) glDeleteBuffers(1, &VBO_ID);
    if (VAO_ID) glDeleteVertexArrays(1, &VAO_ID);

    ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// clean-up GLFW
	if (window) {
		glfwDestroyWindow(window);
		window = nullptr;
	}
	glfwTerminate();

    // clean-up
    cv::destroyAllWindows();
    std::cout << "Bye...\n";
}


void App::init_glfw(void)
{

	/* Initialize the library */
	glfwSetErrorCallback(glfw_error_callback);

	if (!glfwInit()) {
		throw std::runtime_error("GLFW can not be initialized.");
	}

	// try to open OpenGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// open window, but hidden - it will be enabled later, after asset initialization
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(800, 600, "ICP", nullptr, nullptr);
	if (!window) {
		throw std::runtime_error("GLFW window can not be created.");
	}

	glfwSetWindowUserPointer(window, this);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// disable mouse cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLFW callbacks registration
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
	glfwSetKeyCallback(window, glfw_key_callback);
	glfwSetScrollCallback(window, glfw_scroll_callback);
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
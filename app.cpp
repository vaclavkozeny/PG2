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
//---------------------------------------------------------------------
#include "app.hpp"

App::App()
{
    // default constructor
    // nothing to do here (so far...)
    std::cout << "Constructed...\n";
}

void App::initOpenGL() {
    if (!glfwInit()) throw std::runtime_error("GLFW init failed");

    // Na Macu musíme explicitně říct, že chceme Core Profile 4.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(800, 600, "OpenGL context", NULL, NULL);
    if (!window) throw std::runtime_error("Window creation failed");
    
    glfwMakeContextCurrent(window);
    
    glewExperimental = GL_TRUE; // Důležité pro Core Profile na Macu
    glewInit();

    // init glfw
    // https://www.glfw.org/documentation.html
    // TODO: add error checking!
    //glfwInit();

    // open window (GL canvas) with no special properties
    // https://www.glfw.org/docs/latest/quick.html#quick_create_window
    // TODO: add error checking!
    //window = glfwCreateWindow(800, 600, "OpenGL context", NULL, NULL);
    //glfwMakeContextCurrent(window);
    
    // init glew
    // http://glew.sourceforge.net/basic.html
    // TODO: add error checking!
    //glewInit();
    //wglewInit();

    //if (!GLEW_ARB_direct_state_access)
    //    throw std::runtime_error("No DSA :-(");
        
        
    //TODO: get info about your GL context    
}

bool App::init()
{
    try {
        // GL init
        initOpenGL();
            
        init_assets();  
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
        
        while (!glfwWindowShouldClose(window)) {
            // clear canvas
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //set uniform parameter for shader
            // (try to change the color in key callback)          
            glUniform4f(uniform_color_location, r, g, b, a);
            
            //bind 3d object data
            glBindVertexArray(VAO_ID);

            // draw all VAO data
            glDrawArrays(GL_TRIANGLES, 0, triangle_vertices.size());

            // poll events, call callbacks, flip back<->front buffer
            glfwPollEvents();
            glfwSwapBuffers(window);    
        }
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << "Finished OK...\n";
    return EXIT_SUCCESS;
}

App::~App()
{
    //new stuff: cleanup GL data
    if (shader_prog_ID) glDeleteProgram(shader_prog_ID);
    if (VBO_ID) glDeleteBuffers(1, &VBO_ID);
    if (VAO_ID) glDeleteVertexArrays(1, &VAO_ID);

    // clean-up
    cv::destroyAllWindows();
    std::cout << "Bye...\n";
}

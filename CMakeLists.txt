cmake_minimum_required(VERSION 3.30)

project(icp VERSION 2025.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

find_package(glm           CONFIG REQUIRED)
find_package(GLEW                 REQUIRED)
find_package(glfw3         CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui         CONFIG REQUIRED)

if(WIN32)
    find_package(OpenCV CONFIG REQUIRED PATHS "<path to your opencv>")
elseif(UNIX)
    find_package(OpenCV CONFIG REQUIRED)
endif()

set(PROJECT_SOURCES
    src/main.cpp
    src/app.cpp
    src/app.hpp
    src/callbacks.cpp
    src/fps_meter.hpp
    src/non_copyable.hpp
    src/assets.hpp
    src/gl_err_callback.cpp
    src/gl_err_callback.hpp
    src/glerror.cpp
    src/glerror.hpp
    src/ShaderProgram.cpp
    src/ShaderProgram.hpp
    src/Mesh.hpp
    src/OBJloader.cpp
    src/OBJloader.hpp
    src/meshgen.cpp
    src/Model.hpp
    src/Camera.hpp
    src/Texture.cpp
    src/Texture.hpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    GLEW::GLEW
    glfw
    glm::glm
    imgui::imgui
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

# Copy shader files to build directory after every build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/shaders/basic.vert
        ${CMAKE_SOURCE_DIR}/shaders/basic.frag
        ${CMAKE_SOURCE_DIR}/shaders/GL_rainbow.frag
        ${CMAKE_SOURCE_DIR}/shaders/tex.vert
        ${CMAKE_SOURCE_DIR}/shaders/tex.frag
        ${CMAKE_SOURCE_DIR}/shaders/lighting.vert
        ${CMAKE_SOURCE_DIR}/shaders/lighting.frag
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying shaders to build directory"
)

# Note: textures live in <project_root>/resources/textures/ and are loaded
# via the relative path "../resources/textures/" from the build directory.

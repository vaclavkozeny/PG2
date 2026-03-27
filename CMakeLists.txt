cmake_minimum_required(VERSION 3.30)

project(icp VERSION 2025.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

find_package(glm           CONFIG REQUIRED)
find_package(GLEW                 REQUIRED)
find_package(glfw3         CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

find_package(imgui CONFIG REQUIRED)

if(WIN32)
    find_package(OpenCV CONFIG REQUIRED PATHS "<path to your opencv>")
elseif(UNIX)
    find_package(OpenCV CONFIG REQUIRED)
endif()

set(PROJECT_SOURCES
    main.cpp
    app.cpp
    app.hpp
    gl_err_callback.cpp
    glerror.cpp
    callbacks.cpp
    fps_meter.hpp
    non_copyable.hpp
    ShaderProgram.cpp
    ShaderProgram.hpp
    Mesh.hpp
    OBJloader.cpp
    OBJloader.hpp
    meshgen.cpp
    Model.hpp
    Texture.cpp
    Texture.hpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

# Copy shader files to build directory after every build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/basic.vert
        ${CMAKE_SOURCE_DIR}/basic.frag
        ${CMAKE_SOURCE_DIR}/GL_rainbow.frag
        ${CMAKE_SOURCE_DIR}/tex.vert
        ${CMAKE_SOURCE_DIR}/tex.frag
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying shaders to build directory"
)

# Note: textures live in <project_root>/resources/textures/ and are loaded
# via the relative path "../resources/textures/" from the build directory.
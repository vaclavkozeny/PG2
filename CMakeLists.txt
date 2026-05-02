cmake_minimum_required(VERSION 3.30)

project(icp VERSION 2025.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

find_package(glm           CONFIG REQUIRED)
find_package(GLEW                 REQUIRED)
find_package(glfw3         CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(imgui REQUIRED)

if(WIN32)
    find_package(OpenCV CONFIG REQUIRED PATHS "<path to your opencv>")
elseif(UNIX)
    find_package(OpenCV CONFIG REQUIRED)
endif()

set(PROJECT_SOURCES
    src/texture.hpp
    src/texture.cpp
    src/camera.hpp
    src/OBJloader.cpp
    src/OBJloader.hpp
    src/meshgen.cpp
    src/model.hpp
    src/mesh.hpp
    src/shader_program.hpp
    src/shader_program.cpp
    src/fps_meter.hpp
    src/gl_err_callback.h
    src/gl_err_callback.cpp
    src/getinfo.cpp
    src/getinfo.hpp
    src/assets.hpp
    src/app.hpp
    src/app.cpp
    src/main.cpp
)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    OpenGL::GL
    GLEW::GLEW
    glfw
    glm::glm
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
    imgui::imgui
)


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)
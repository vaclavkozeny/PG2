#include <iostream>

#include <glm/glm.hpp>

#include "app.hpp"

void App::glfw_error_callback(int /*error*/, const char* description) {
    std::cerr << "GLFW error: " << description << '\n';
}

void App::glfw_key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;

        case GLFW_KEY_SPACE:
            // Jump — consumed by update_physics(); buffered as one-shot flag
            app->player.jump_req = true;
            break;

        case GLFW_KEY_R:
            if (action == GLFW_PRESS) {
                // Restart the course
                app->phase        = GamePhase::Playing;
                app->elapsed_time = 0.0f;
                app->death_count  = 0;
                app->respawn();
            }
            break;

        // Vsync toggle
        case GLFW_KEY_V:
            if (action == GLFW_PRESS) {
                app->vsync = !app->vsync;
                glfwSwapInterval(app->vsync ? 1 : 0);
                std::cout << "VSync: " << (app->vsync ? "ON" : "OFF") << '\n';
            }
            break;

        case GLFW_KEY_F:
            if (action == GLFW_PRESS)
                app->toggle_fullscreen(window);
            break;

        case GLFW_KEY_I:
            if (action == GLFW_PRESS)
                app->show_imgui = !app->show_imgui;
            break;

        case GLFW_KEY_H:
            if (action == GLFW_PRESS) {
                app->spotLight.on = !app->spotLight.on;
                std::cout << "Headlight: " << (app->spotLight.on ? "ON" : "OFF") << '\n';
            }
            break;

        default:
            break;
        }
    }
}

void App::glfw_scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    app->fov = std::clamp(app->fov - static_cast<float>(yoffset) * 3.0f, 20.0f, 120.0f);
    app->update_projection_matrix();
}

void App::glfw_framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    app->width  = w;
    app->height = h;
    glViewport(0, 0, w, h);
    app->update_projection_matrix();
}

void App::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            // Recapture cursor if it was released
            if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            // Release cursor so the player can interact with OS / ImGui
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void App::glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));

    const double dx = xpos - app->cursorLastX;
    const double dy = app->cursorLastY - ypos;  // y grows downward in screen space
    app->cursorLastX = xpos;
    app->cursorLastY = ypos;

    // Only rotate camera when cursor is captured
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        app->camera.ProcessMouseMovement(static_cast<float>(dx), static_cast<float>(dy));
}

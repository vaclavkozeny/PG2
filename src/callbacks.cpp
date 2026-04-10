#include <iostream>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "app.hpp"

void App::glfw_error_callback(int error, const char* description)
{
	std::cerr << "GLFW error: " << description << std::endl;
}

void App::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));
	if ((action == GLFW_PRESS) || (action == GLFW_REPEAT)) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			// Exit The App
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		case GLFW_KEY_V:
			if (action == GLFW_PRESS) {  // Only toggle on press
				// Vsync on/off
				this_inst->vsync = !this_inst->vsync;
				glfwSwapInterval(this_inst->vsync);
				std::cout << "VSync: " << (this_inst->vsync ? "ON" : "OFF") << "\n";
			}
			break;
		case GLFW_KEY_I:
			if (action == GLFW_PRESS)
				this_inst->show_imgui = !this_inst->show_imgui;
			break;
		case GLFW_KEY_F:
			if (action == GLFW_PRESS) {  // Only toggle on press
				this_inst->toggle_fullscreen(window);
			}
			break;
		case GLFW_KEY_H:
			if (action == GLFW_PRESS) {
				this_inst->spotLight.on = !this_inst->spotLight.on;
				std::cout << "Headlight: " << (this_inst->spotLight.on ? "ON" : "OFF") << "\n";
			}
			break;
		default:
			break;
		}
	}
}

void App::glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));
    
    // Change FOV with mouse wheel  
    this_inst->fov += yoffset * 5.0f;
    this_inst->fov = std::clamp(this_inst->fov, 20.0f, 170.0f);
    this_inst->update_projection_matrix();
    
    std::cout << "FOV: " << this_inst->fov << "°\n";
}

void App::glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));
    this_inst->width = width;
    this_inst->height = height;
    
    glViewport(0, 0, width, height);
    this_inst->update_projection_matrix();
}

void App::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (action == GLFW_PRESS) {
		switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT: {
			int mode = glfwGetInputMode(window, GLFW_CURSOR);
			if (mode == GLFW_CURSOR_NORMAL) {
				// we are outside of application, catch the cursor
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			else {
				// we are already inside our game: shoot, click, etc.
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

void App::glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));
    
    // Calculate offset
    double xoffset = xpos - this_inst->cursorLastX;
    double yoffset = this_inst->cursorLastY - ypos;  // Reversed: y increases downward in pixel space
    
    // Update last position
    this_inst->cursorLastX = xpos;
    this_inst->cursorLastY = ypos;
    
    // Process mouse movement for camera
    this_inst->camera.ProcessMouseMovement(static_cast<GLfloat>(xoffset), static_cast<GLfloat>(yoffset));
}

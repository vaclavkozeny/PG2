#pragma once

#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    // Camera Attributes
    glm::vec3 Position{0.0f, 0.0f, 0.0f};
    glm::vec3 Front{0.0f, 0.0f, -1.0f};
    glm::vec3 Right{1.0f, 0.0f, 0.0f}; 
    glm::vec3 Up{0.0f, 1.0f, 0.0f}; // camera local UP vector

    GLfloat Yaw = -90.0f;
    GLfloat Pitch = 0.0f;
    GLfloat Roll = 0.0f;
    
    // Camera options
    GLfloat MovementSpeed = 5.0f;
    GLfloat MouseSensitivity = 0.25f;
    
    // Default constructor
    Camera() {
        this->updateCameraVectors();
    }

    // Constructor with position
    Camera(glm::vec3 position) : Position(position) {
        this->Up = glm::vec3(0.0f, 1.0f, 0.0f);
        // initialization of the camera reference system
        this->updateCameraVectors();
    }

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(this->Position, this->Position + this->Front, this->Up);
    }

    // Processes input received from any keyboard-like input system. 
    // Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    glm::vec3 ProcessInput(GLFWwindow* window, GLfloat deltaTime) {
        glm::vec3 direction{0};
          
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            direction += Front; // add unit vector to final direction  

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            direction -= Front;

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            direction -= Right;       

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            direction += Right;

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            direction += Up;

        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            direction -= Up;

        // Normalize direction and multiply by speed and delta time
        if (glm::length(direction) > 0.0f) {
            direction = glm::normalize(direction);
        }
        
        return direction * MovementSpeed * deltaTime;
    }

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constraintPitch = GL_TRUE) {
        xoffset *= this->MouseSensitivity;
        yoffset *= this->MouseSensitivity;

        this->Yaw   += xoffset;
        this->Pitch += yoffset;

        // make sure that when Pitch is out of bounds, screen doesn't get flipped
        if (constraintPitch) {
            if (this->Pitch > 89.0f)
                this->Pitch = 89.0f;
            if (this->Pitch < -89.0f)
                this->Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        this->updateCameraVectors();
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors() {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
        front.y = sin(glm::radians(this->Pitch));
        front.z = sin(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));

        this->Front = glm::normalize(front);
        
        // also re-calculate the Right and Up vector
        // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        this->Right = glm::normalize(glm::cross(this->Front, glm::vec3(0.0f, 1.0f, 0.0f)));
        this->Up = glm::normalize(glm::cross(this->Right, this->Front));
    }
};

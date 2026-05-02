#pragma once

#include <glm/glm.hpp>

// Holds all physics and state for the first-person parkour player.
// The camera is placed at eye_pos() every frame.
struct Player {
    static constexpr float EYE_HEIGHT = 1.6f;   // camera above feet (metres)
    static constexpr float RADIUS     = 0.3f;   // horizontal bounding radius
    static constexpr float GRAVITY    = -20.0f; // downward acceleration (m/s²)
    static constexpr float JUMP_SPEED = 8.0f;   // initial upward velocity on jump
    static constexpr float MOVE_SPEED = 5.0f;   // horizontal speed (world units/s)
    static constexpr float DEATH_Y    = -6.0f;  // fall below this → respawn

    glm::vec3 feet   {0.0f, 0.5f, 0.0f}; // bottom of player bounding box (world)
    float     vel_y  {0.0f};             // current vertical velocity (m/s)
    bool      grounded{false};           // true when standing on a block
    bool      jump_req{false};           // set by key callback, consumed by physics

    glm::vec3 eye_pos() const { return feet + glm::vec3(0.0f, EYE_HEIGHT, 0.0f); }
    bool      is_dead()  const { return feet.y < DEATH_Y; }
};
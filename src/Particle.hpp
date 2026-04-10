#pragma once

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.hpp"
#include "ShaderProgram.hpp"

// ============================================================
// Single particle data
// ============================================================

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float     lifetime;
    float     maxLifetime;
};

// ============================================================
// Particle system
//
// Usage:
//   1. Call init() with a shared mesh (tetrahedron, point, etc.)
//   2. Call emit() when an event fires (collision, explosion, etc.)
//   3. Call update(dt) every frame
//   4. Call draw() inside the transparent/blended render pass
// ============================================================

class ParticleSystem {
public:
    static constexpr float GRAVITY     = -9.8f;   // m/s²
    static constexpr float FLOOR_Y     = 0.02f;   // bounce plane
    static constexpr float BOUNCE_DAMP = 0.45f;   // energy kept on Y-bounce
    static constexpr float LATERAL_DAMP = 0.88f;  // energy kept on XZ-bounce
    static constexpr float PARTICLE_SIZE = 0.08f; // scale of each tetrahedron

    // Assign the shared mesh used to draw every particle
    void init(std::shared_ptr<Mesh> mesh) {
        mesh_ = std::move(mesh);
    }

    // Spawn `count` particles at `origin` with given color/speed/lifetime
    void emit(const glm::vec3& origin,
              const glm::vec3& color,
              int   count,
              float speed,
              float lifetime = 1.5f)
    {
        for (int i = 0; i < count; ++i) {
            Particle p;
            p.position    = origin;
            p.color       = color;
            // Vary lifetime slightly so particles don't all die at once
            p.maxLifetime = lifetime * (0.6f + 0.8f * rf());
            p.lifetime    = p.maxLifetime;

            // Random direction on upper hemisphere (bias upward for
            // a natural "burst" effect)
            float theta = rf() * 2.0f * glm::pi<float>();
            float phi   = rf() * glm::half_pi<float>(); // 0..π/2 → upper only
            float s     = speed * (0.4f + 0.6f * rf());
            p.velocity = glm::vec3(
                glm::sin(phi) * glm::cos(theta) * s,
                glm::cos(phi) * s,
                glm::sin(phi) * glm::sin(theta) * s
            );
            particles_.push_back(p);
        }
    }

    // Advance simulation by dt seconds
    void update(float dt) {
        for (auto& p : particles_) {
            p.velocity.y += GRAVITY * dt;
            p.position   += p.velocity * dt;

            // Bounce on floor
            if (p.position.y < FLOOR_Y) {
                p.position.y  = FLOOR_Y;
                p.velocity.y  = -p.velocity.y * BOUNCE_DAMP;
                p.velocity.x *= LATERAL_DAMP;
                p.velocity.z *= LATERAL_DAMP;
            }

            p.lifetime -= dt;
        }

        // Erase dead particles
        particles_.erase(
            std::remove_if(particles_.begin(), particles_.end(),
                [](const Particle& p) { return p.lifetime <= 0.0f; }),
            particles_.end()
        );
    }

    // Draw all live particles using the particle shader.
    // Must be called inside a blending-enabled render pass.
    void draw(ShaderProgram& shader,
              const glm::mat4& V,
              const glm::mat4& P) const
    {
        if (particles_.empty() || !mesh_) return;

        shader.use();
        shader.setUniform("uV_m", V);
        shader.setUniform("uP_m", P);

        for (const auto& p : particles_) {
            // Fade out as lifetime approaches 0
            float alpha = glm::max(p.lifetime / p.maxLifetime, 0.0f);

            glm::mat4 M = glm::scale(
                glm::translate(glm::mat4(1.0f), p.position),
                glm::vec3(PARTICLE_SIZE)
            );
            shader.setUniform("uM_m",   M);
            shader.setUniform("uColor", glm::vec4(p.color, alpha));
            mesh_->draw();
        }
    }

    bool   empty() const { return particles_.empty(); }
    size_t count() const { return particles_.size(); }

private:
    std::vector<Particle> particles_;
    std::shared_ptr<Mesh> mesh_;

    // [0, 1) uniform random float
    static float rf() {
        return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    }
};

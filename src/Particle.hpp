#pragma once

#include <algorithm>
#include <memory>
#include <random>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.hpp"
#include "ShaderProgram.hpp"

// ============================================================
// Single particle — position/velocity/color/lifetime.
// ============================================================
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float     lifetime;     // remaining seconds
    float     maxLifetime;  // total seconds at spawn
};

// ============================================================
// Particle system
//
// Uses a shared mesh (e.g. tetrahedron) for all particles so
// GPU memory stays constant regardless of particle count.
//
// Usage:
//   1. init(mesh)          — assign the shared draw mesh
//   2. emit(...)           — fire a burst on a game event
//   3. update(dt)          — call once per frame
//   4. draw(shader, V, P)  — call inside the blended pass
// ============================================================
class ParticleSystem {
public:
    // Physics constants
    static constexpr float GRAVITY      = -9.8f;
    static constexpr float FLOOR_Y      =  0.02f;
    static constexpr float BOUNCE_DAMP  =  0.45f;   // Y energy kept on bounce
    static constexpr float LATERAL_DAMP =  0.88f;   // XZ friction on bounce
    static constexpr float DRAW_SCALE   =  0.08f;   // uniform scale of each particle

    // Assign the mesh drawn for every particle (call once at startup).
    void init(std::shared_ptr<Mesh> mesh) {
        mesh_ = std::move(mesh);
    }

    // Spawn `count` particles at `origin`.
    // `speed`    — initial speed magnitude (units/s)
    // `lifetime` — mean lifetime in seconds (varied ±40 % per particle)
    void emit(const glm::vec3& origin,
              const glm::vec3& color,
              int   count,
              float speed,
              float lifetime = 1.5f)
    {
        particles_.reserve(particles_.size() + count);
        for (int i = 0; i < count; ++i) {
            Particle p;
            p.position    = origin;
            p.color       = color;
            p.maxLifetime = lifetime * (0.6f + 0.8f * unit_rand());
            p.lifetime    = p.maxLifetime;
            p.velocity    = random_upper_hemisphere(speed);
            particles_.push_back(p);
        }
    }

    // Step physics by dt seconds; remove dead particles.
    void update(float dt) {
        for (auto& p : particles_) {
            p.velocity.y += GRAVITY * dt;
            p.position   += p.velocity * dt;

            if (p.position.y < FLOOR_Y) {
                p.position.y  = FLOOR_Y;
                p.velocity.y  = -p.velocity.y * BOUNCE_DAMP;
                p.velocity.x *= LATERAL_DAMP;
                p.velocity.z *= LATERAL_DAMP;
            }

            p.lifetime -= dt;
        }

        std::erase_if(particles_, [](const Particle& p) {
            return p.lifetime <= 0.0f;
        });
    }

    // Render all live particles.  Must be called with blending enabled.
    // Expects uniforms: uM_m, uV_m, uP_m, uColor (vec4).
    void draw(ShaderProgram& shader,
              const glm::mat4& V,
              const glm::mat4& P) const
    {
        if (particles_.empty() || !mesh_) return;

        shader.use();
        shader.setUniform("uV_m", V);
        shader.setUniform("uP_m", P);

        for (const auto& p : particles_) {
            const float alpha = glm::max(p.lifetime / p.maxLifetime, 0.0f);
            const glm::mat4 M = glm::scale(
                glm::translate(glm::mat4(1.0f), p.position),
                glm::vec3(DRAW_SCALE)
            );
            shader.setUniform("uM_m",   M);
            shader.setUniform("uColor", glm::vec4(p.color, alpha));
            mesh_->draw();
        }
    }

    [[nodiscard]] bool   empty() const { return particles_.empty(); }
    [[nodiscard]] size_t count() const { return particles_.size(); }

private:
    std::vector<Particle> particles_;
    std::shared_ptr<Mesh> mesh_;

    // Seeded once per system — no global state, no race conditions.
    std::mt19937 rng_{std::random_device{}()};
    std::uniform_real_distribution<float> dist_{0.0f, 1.0f};

    // [0, 1) uniform float
    float unit_rand() { return dist_(rng_); }

    // Random direction on the upper hemisphere, scaled by `speed`
    // (±40 % speed variance for a natural burst spread).
    glm::vec3 random_upper_hemisphere(float speed) {
        const float theta = unit_rand() * 2.0f * glm::pi<float>();
        const float phi   = unit_rand() * glm::half_pi<float>();
        const float s     = speed * (0.6f + 0.8f * unit_rand());
        return {
            glm::sin(phi) * glm::cos(theta) * s,
            glm::cos(phi) * s,
            glm::sin(phi) * glm::sin(theta) * s,
        };
    }
};

#include "Renderer.h"
#include "Physics.h"
#include "Camera.h"
#include "Grid.h"
#include "QuantumState.h"
#include <iostream>
#include <vector>

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods;
    if (!(action == GLFW_PRESS || action == GLFW_REPEAT)) return;

    UserPointer* up = (UserPointer*)glfwGetWindowUserPointer(window);
    QuantumState& state = *up->state;
    std::vector<Particle>& particles = *up->particles;

    bool updated = false;

    if (key == GLFW_KEY_W) {
        state.n += 1;
        updated = true;
    } else if (key == GLFW_KEY_S) {
        state.n -= 1;
        if (state.n < 1) state.n = 1;
        updated = true;
    } else if (key == GLFW_KEY_E) {
        state.l += 1;
        updated = true;
    } else if (key == GLFW_KEY_D) {
        state.l -= 1;
        if (state.l < 0) state.l = 0;
        updated = true;
    } else if (key == GLFW_KEY_R) {
        state.m += 1;
        updated = true;
    } else if (key == GLFW_KEY_F) {
        state.m -= 1;
        updated = true;
    } else if (key == GLFW_KEY_T) {
        state.N += 100000;
        updated = true;
    } else if (key == GLFW_KEY_G) {
        state.N -= 100000;
        if (state.N < 100000) state.N = 100000;
        updated = true;
    }

    if (updated) {
        // Clamp to valid ranges
        if (state.l > state.n - 1) state.l = state.n - 1;
        if (state.l < 0) state.l = 0;
        if (state.m > state.l) state.m = state.l;
        if (state.m < -state.l) state.m = -state.l;

        state.updateElectronR();
        Physics::generateParticles(particles, state);
        std::cout << "Quantum numbers updated: n=" << state.n << " l=" << state.l << " m=" << state.m << " N=" << state.N << "\n";
    }
}

int main() {
    QuantumState state;
    Camera camera;
    Renderer renderer;
    std::vector<Particle> particles;

    renderer.init(&camera, &state);

    if (!renderer.window) {
        std::cerr << "Window creation failed!\n";
        return -1;
    }

    renderer.setParticles(&particles);
    glfwSetKeyCallback(renderer.window, keyCallback);

    Grid grid(renderer);

    state.updateElectronR();
    Physics::generateParticles(particles, state);
    std::cout << "Generated " << particles.size() << " particles." << std::endl;

    float dt = 0.5f;
    std::cout << "Starting simulation..." << std::endl;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int frameCount = 0;
    while (!glfwWindowShouldClose(renderer.window)) {
        if (frameCount % 100 == 0) {
            std::cout << "Frame: " << frameCount << std::endl;
        }
        frameCount++;

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ------ Update Probability current ------
        for (Particle& p : particles) {
            double r = glm::length(p.pos);
            if (r > 1e-6) {
                double theta = std::acos(p.pos.y / r);
                p.vel = Physics::calculateProbabilityFlow(p, state);
                glm::vec3 temp_pos = p.pos + p.vel * dt;
                double new_phi = std::atan2(temp_pos.z, temp_pos.x);
                p.pos = Physics::sphericalToCartesian((float)r, (float)theta, (float)new_phi);
            }
        }

        // ------ Draw Particles ------
        renderer.drawSpheres(particles, camera, state);

        glfwSwapBuffers(renderer.window);
        glfwPollEvents();
    }

    return 0;
}

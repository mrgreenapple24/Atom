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

    state.n = 3; state.l = 2; state.m = 0; // More complex orbital
    state.updateElectronR();
    Physics::generateParticles(particles, state);
    std::cout << "Generated " << particles.size() << " particles." << std::endl;

    std::cout << "Starting simulation..." << std::endl;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    double lastTime = glfwGetTime();
    int frameCount = 0;
    while (!glfwWindowShouldClose(renderer.window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        if (frameCount % 100 == 0) {
            std::cout << "Frame: " << frameCount << " | FPS: " << (1.0f / deltaTime) << std::endl;
        }
        frameCount++;

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ------ Update Probability current ------
        for (Particle& p : particles) {
            p.vel = Physics::calculateProbabilityFlow(p, state);
            p.pos += p.vel * 0.1f; // Use a tuned constant for observable motion speed
        }

        // ------ Draw Particles ------
        renderer.drawSpheres(particles, camera, state);

        // ------ Draw UI ------
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(renderer.window, &fbWidth, &fbHeight);
        float line_height = 20.0f;
        float x_start = 10.0f;
        float y_start = (float)fbHeight - 25.0f;

        renderer.drawText("Controls:", x_start, y_start, 2.0f);
        renderer.drawText("W/S: n = " + std::to_string(state.n), x_start, y_start - line_height * 1, 1.5f);
        renderer.drawText("E/D: l = " + std::to_string(state.l), x_start, y_start - line_height * 2, 1.5f);
        renderer.drawText("R/F: m = " + std::to_string(state.m), x_start, y_start - line_height * 3, 1.5f);
        renderer.drawText("T/G: N = " + std::to_string(state.N), x_start, y_start - line_height * 4, 1.5f);

        glfwSwapBuffers(renderer.window);
        glfwPollEvents();
    }

    return 0;
}

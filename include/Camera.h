#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Camera {
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    float radius = 50.0f;
    float azimuth = 0.0f;
    float elevation = M_PI / 2.0f;
    float orbitSpeed = 0.01f;
    float panSpeed = 0.01f;
    double zoomSpeed = 10.0;
    bool dragging = false;
    bool panning = false;
    double lastX = 0.0, lastY = 0.0;

    glm::vec3 position() const;
    void update();
    void processMouseMove(double x, double y);
    void processMouseButton(int button, int action, int mods, GLFWwindow* win);
    void processScroll(double xoffset, double yoffset);
};

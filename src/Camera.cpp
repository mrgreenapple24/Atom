#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

glm::vec3 Camera::position() const {
    float clampedElevation = std::clamp(elevation, 0.01f, float(M_PI) - 0.01f);
    return glm::vec3(
        radius * std::sin(clampedElevation) * std::cos(azimuth),
        radius * std::cos(clampedElevation),
        radius * std::sin(clampedElevation) * std::sin(azimuth)
    );
}

void Camera::update() {
    target = glm::vec3(0.0f, 0.0f, 0.0f);
}

void Camera::processMouseMove(double x, double y) {
    float dx = float(x - lastX);
    float dy = float(y - lastY);
    if (dragging) {
        azimuth += dx * orbitSpeed;
        elevation -= dy * orbitSpeed;
        elevation = std::clamp(elevation, 0.01f, float(M_PI) - 0.01f);
    }
    lastX = x;
    lastY = y;
    update();
}

void Camera::processMouseButton(int button, int action, int mods, GLFWwindow* win) {
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            dragging = true;
            glfwGetCursorPos(win, &lastX, &lastY);
        } else if (action == GLFW_RELEASE) {
            dragging = false;
        }
    }
}

void Camera::processScroll(double xoffset, double yoffset) {
    (void)xoffset;
    radius -= (float)(yoffset * zoomSpeed);
    if (radius < 1.0f) radius = 1.0f;
    update();
}

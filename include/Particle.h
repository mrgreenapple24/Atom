#pragma once
#include <glm/glm.hpp>

struct Particle {
    glm::vec3 pos;
    glm::vec3 vel = glm::vec3(0.0f);
    glm::vec4 color;

    Particle() : pos(0.0f), color(0.0f) {}
    Particle(glm::vec3 p, glm::vec4 c = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f)) 
        : pos(p), color(c) {}
};

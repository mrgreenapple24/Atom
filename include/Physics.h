#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "QuantumState.h"
#include "Particle.h"

namespace Physics {
    double sampleR(const QuantumState& state);
    double sampleTheta(const QuantumState& state);
    float samplePhi(const QuantumState& state);
    
    glm::vec3 calculateProbabilityFlow(const Particle& p, const QuantumState& state);
    glm::vec4 inferno(double r, double theta, double phi, const QuantumState& state);
    glm::vec4 heatmap_fire(float value);

    glm::vec3 sphericalToCartesian(float r, float theta, float phi);
    void generateParticles(std::vector<Particle>& particles, QuantumState& state);
}

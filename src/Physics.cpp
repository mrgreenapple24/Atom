#include "Physics.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Physics {

double sampleR(const QuantumState& state) {
    const int N_bins = 4096;
    const double rMax = 10.0 * state.n * state.n * state.a0;

    std::vector<double> cdf(N_bins);
    double dr = rMax / (N_bins - 1);
    double sum = 0.0;

    for (int i = 0; i < N_bins; ++i) {
        double r = i * dr;
        double rho = 2.0 * r / (state.n * state.a0);

        int k = state.n - state.l - 1;
        int alpha = 2 * state.l + 1;

        double L = 1.0, Lm1 = 1.0 + alpha - rho;
        if (k == 1) L = Lm1;
        else if (k > 1) {
            double Lm2 = 1.0;
            for (int j = 2; j <= k; ++j) {
                L = ((2*j - 1 + alpha - rho) * Lm1 - (j - 1 + alpha) * Lm2) / j;
                Lm2 = Lm1;
                Lm1 = L;
            }
        }

        double norm = std::pow(2.0 / (state.n * state.a0), 3) * std::tgamma(state.n - state.l) / (2.0 * state.n * std::tgamma(state.n + state.l + 1));
        double R = std::sqrt(norm) * std::exp(-rho / 2.0) * std::pow(rho, state.l) * L;

        double pdf = r * r * R * R;
        sum += pdf;
        cdf[i] = sum;
    }

    for (double& v : cdf) v /= sum;

    std::uniform_real_distribution<double> local_dis(0.0, 1.0);
    double u = local_dis(const_cast<QuantumState&>(state).gen);

    int idx = std::lower_bound(cdf.begin(), cdf.end(), u) - cdf.begin();
    return idx * (rMax / (N_bins - 1));
}

double sampleTheta(const QuantumState& state) {
    const int N_bins = 2048;
    std::vector<double> cdf(N_bins);
    double dtheta = M_PI / (N_bins - 1);
    double sum = 0.0;

    for (int i = 0; i < N_bins; ++i) {
        double theta = i * dtheta;
        double x = std::cos(theta);

        double Pmm = 1.0;
        if (state.m > 0) {
            double somx2 = std::sqrt((1.0 - x) * (1.0 + x));
            double fact = 1.0;
            for (int j = 1; j <= state.m; ++j) {
                Pmm *= -fact * somx2;
                fact += 2.0;
            }
        }

        double Plm;
        if (state.l == state.m) {
            Plm = Pmm;
        } else {
            double Pm1m = x * (2 * state.m + 1) * Pmm;
            if (state.l == state.m + 1) {
                Plm = Pm1m;
            } else {
                double Pll;
                double local_Pmm = Pmm;
                double local_Pm1m = Pm1m;
                for (int ll = state.m + 2; ll <= state.l; ++ll) {
                    Pll = ((2 * ll - 1) * x * local_Pm1m - (ll + state.m - 1) * local_Pmm) / (ll - state.m);
                    local_Pmm = local_Pm1m;
                    local_Pm1m = Pll;
                }
                Plm = local_Pm1m;
            }
        }

        double pdf = std::sin(theta) * Plm * Plm;
        sum += pdf;
        cdf[i] = sum;
    }

    for (double& v : cdf) v /= sum;

    std::uniform_real_distribution<double> local_dis(0.0, 1.0);
    double u = local_dis(const_cast<QuantumState&>(state).gen);

    int idx = std::lower_bound(cdf.begin(), cdf.end(), u) - cdf.begin();
    return idx * (M_PI / (N_bins - 1));
}

float samplePhi(const QuantumState& state) {
    std::uniform_real_distribution<float> local_dis(0.0f, 1.0f);
    return 2.0f * M_PI * local_dis(const_cast<QuantumState&>(state).gen);
}

glm::vec3 calculateProbabilityFlow(const Particle& p, const QuantumState& state) {
    (void)p;

    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);
    float rx = dist(const_cast<QuantumState&>(state).gen);
    float ry = dist(const_cast<QuantumState&>(state).gen);
    float rz = dist(const_cast<QuantumState&>(state).gen);

    return glm::vec3(rx, ry, rz);
}


glm::vec4 heatmap_fire(float value) {
    value = std::max(0.0f, std::min(1.0f, value));
    const int num_stops = 5;
    glm::vec4 colors[num_stops] = {
        {0.0f, 0.0f, 0.5f, 1.0f}, // Dark Blue
        {0.0f, 1.0f, 1.0f, 1.0f}, // Cyan
        {0.0f, 1.0f, 0.0f, 1.0f}, // Green
        {1.0f, 1.0f, 0.0f, 1.0f}, // Yellow
        {1.0f, 0.0f, 0.0f, 1.0f}  // Red
    };

    float scaled_v = value * (num_stops - 1);
    int i = static_cast<int>(scaled_v);
    int next_i = std::min(i + 1, num_stops - 1);
    float local_t = scaled_v - i;

    return glm::mix(colors[i], colors[next_i], local_t);
}

glm::vec4 inferno(double r, double theta, double phi, const QuantumState& state) {
    (void)phi; // Currently unused in this intensity calculation
    double rho = 2.0 * r / (state.n * state.a0);

    int k = state.n - state.l - 1;
    int alpha = 2 * state.l + 1;

    double L = 1.0;
    if (k == 1) {
        L = 1.0 + alpha - rho;
    } else if (k > 1) {
        double Lm2 = 1.0;
        double Lm1 = 1.0 + alpha - rho;
        for (int j = 2; j <= k; ++j) {
            L = ((2*j - 1 + alpha - rho) * Lm1 - (j - 1 + alpha) * Lm2) / j;
            Lm2 = Lm1;
            Lm1 = L;
        }
    }

    double norm = std::pow(2.0 / (state.n * state.a0), 3) * std::tgamma(state.n - state.l) / (2.0 * state.n * std::tgamma(state.n + state.l + 1));
    double R = std::sqrt(norm) * std::exp(-rho / 2.0) * std::pow(rho, state.l) * L;
    double radial = R * R;

    double x = std::cos(theta);
    double Pmm = 1.0;
    if (state.m > 0) {
        double somx2 = std::sqrt((1.0 - x) * (1.0 + x));
        double fact = 1.0;
        for (int j = 1; j <= state.m; ++j) {
            Pmm *= -fact * somx2;
            fact += 2.0;
        }
    }

    double Plm;
    if (state.l == state.m) {
        Plm = Pmm;
    } else {
        double Pm1m = x * (2 * state.m + 1) * Pmm;
        if (state.l == state.m + 1) {
            Plm = Pm1m;
        } else {
            double local_Pmm = Pmm;
            double local_Pm1m = Pm1m;
            for (int ll = state.m + 2; ll <= state.l; ++ll) {
                double Pll = ((2*ll - 1) * x * local_Pm1m - (ll + state.m - 1) * local_Pmm) / (ll - state.m);
                local_Pmm = local_Pm1m;
                local_Pm1m = Pll;
            }
            Plm = local_Pm1m;
        }
    }

    double angularPart;

    if (state.m > 0)
        angularPart = Plm * std::cos(state.m * phi);
    else if (state.m < 0)
        angularPart = Plm * std::sin(std::abs(state.m) * phi);
    else
        angularPart = Plm;

    double angular = angularPart * angularPart;
    double intensity = radial * angular;

    return heatmap_fire(intensity * 1.5 * std::pow(5, state.n));
}

glm::vec3 sphericalToCartesian(float r, float theta, float phi) {
    float x = r * std::sin(theta) * std::cos(phi);
    float y = r * std::cos(theta);
    float z = r * std::sin(theta) * std::sin(phi);
    return glm::vec3(x, y, z);
}

void generateParticles(std::vector<Particle>& particles, QuantumState& state) {
    particles.clear();
    particles.reserve(state.N);
    for (int i = 0; i < state.N; ++i) {
        particles.push_back(sampleParticle(state));
    }
}

Particle sampleParticle(const QuantumState& state) {
    float r_val = (float)sampleR(state);
    float theta_val = (float)sampleTheta(state);
    float phi_val = samplePhi(state);

    glm::vec3 pos = sphericalToCartesian(r_val, theta_val, phi_val);
    glm::vec4 col = inferno(r_val, theta_val, phi_val, state);
    return Particle(pos, col);
}

} // namespace Physics

#pragma once
#include <random>

struct QuantumState {
    int n = 4;
    int l = 3;
    int m = 0;
    int N = 10000;
    float electron_r = 1.5f;
    const float a0 = 1.0f;
    const double hbar = 1.0;
    const double m_e = 1.0;

    std::random_device rd;
    std::mt19937 gen;

    QuantumState() : gen(rd()) {}

    void updateElectronR() {
        electron_r = static_cast<float>(n) / 3.0f;
    }
};

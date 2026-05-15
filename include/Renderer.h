#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include "Camera.h"
#include "Particle.h"
#include "QuantumState.h"

struct UserPointer {
    Camera* camera;
    QuantumState* state;
    std::vector<Particle>* particles;
};

class Renderer {
public:
    GLFWwindow* window;
    int width = 800;
    int height = 600;

    Renderer();
    ~Renderer();

    void init(Camera* camera, QuantumState* state);
    void setParticles(std::vector<Particle>* particles);
    void drawSpheres(const std::vector<Particle>& particles, const Camera& camera, const QuantumState& state);
    void drawText(const std::string& text, float x, float y, float scale);
    void createVBOVAO(GLuint& VAO, GLuint& VBO, const std::vector<float>& vertices);
    void createVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount);

    GLuint shaderProgram;
    GLuint textShaderProgram;
    GLint modelLoc, viewLoc, projLoc, colorLoc;
    GLint textProjLoc, textColorLoc;

private:
    GLuint sphereVAO, sphereVBO;
    int sphereVertexCount;

    GLuint textVAO, textVBO;
    GLuint fontTexture;

    void setupShaders();
    void setupSphere();
    void setupText();
};

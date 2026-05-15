#pragma once
#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>
#include "Renderer.h"

class Grid {
public:
    Grid(Renderer& renderer);
    ~Grid();

    void draw(Renderer& renderer);

private:
    GLuint gridVAO, gridVBO;
    std::vector<float> vertices;

    std::vector<float> createGridVertices(float size, int divisions);
    void drawGrid(GLuint shaderProgram, GLuint vao, size_t vertexCount);
};

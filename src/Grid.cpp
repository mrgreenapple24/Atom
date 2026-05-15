#include "Grid.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

Grid::Grid(Renderer& renderer) {
    vertices = createGridVertices(500.0f, 2);
    renderer.createVBOVAO(gridVAO, gridVBO, vertices.data(), vertices.size());
}

Grid::~Grid() {
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
}

void Grid::draw(Renderer& renderer) {
    glUseProgram(renderer.shaderProgram);
    glUniform4f(renderer.colorLoc, 1.0f, 1.0f, 1.0f, 0.5f);
    
    // Set view and projection (normally these should be set once per frame)
    UserPointer* up = (UserPointer*)glfwGetWindowUserPointer(renderer.window);
    Camera& camera = *up->camera;
    
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)renderer.width/(float)renderer.height, 0.1f, 2000.0f);
    glm::mat4 view = glm::lookAt(camera.position(), camera.target, glm::vec3(0, 1, 0));
    
    glUniformMatrix4fv(renderer.viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(renderer.projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Bind and update buffer if needed
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    
    drawGrid(renderer.shaderProgram, gridVAO, vertices.size());
}

void Grid::drawGrid(GLuint shaderProgram, GLuint vao, size_t vertexCount) {
    glm::mat4 model = glm::mat4(1.0f);
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(vao);
    glPointSize(2.0f);
    glDrawArrays(GL_LINES, 0, (GLsizei)vertexCount / 3);
    glBindVertexArray(0);
}

std::vector<float> Grid::createGridVertices(float size, int divisions) {
    std::vector<float> v;
    float step = size / divisions;
    float halfSize = size / 2.0f;
    float extra = step * 3.0f;
    int midZ = divisions / 2;

    for (int yStep = 3; yStep <= 3; ++yStep) {
        float y = 0;
        for (int zStep = 0; zStep <= divisions; ++zStep) {
            float z = -halfSize + zStep * step;
            for (int xStep = 0; xStep < divisions; ++xStep) {
                float xStart = -halfSize + xStep * step;
                float xEnd = xStart + step;
                if (zStep == midZ) {
                    if (xStep == 0) xStart -= extra;
                    if (xStep == divisions - 1) xEnd += extra;
                }
                v.push_back(xStart); v.push_back(y); v.push_back(z);
                v.push_back(xEnd);   v.push_back(y); v.push_back(z);
            }
        }
    }
    for (int xStep = 0; xStep <= divisions; ++xStep) {
        float x = -halfSize + xStep * step;
        for (int yStep = 3; yStep <= 3; ++yStep) {
            float y = 0;
            for (int zStep = 0; zStep < divisions; ++zStep) {
                float zStart = -halfSize + zStep * step;
                float zEnd = zStart + step;
                v.push_back(x); v.push_back(y); v.push_back(zStart);
                v.push_back(x); v.push_back(y); v.push_back(zEnd);
            }
        }
    }
    return v;
}

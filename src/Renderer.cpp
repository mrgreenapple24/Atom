#include "Renderer.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Physics.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Renderer::Renderer() : window(nullptr), shaderProgram(0), sphereVAO(0), sphereVBO(0), sphereVertexCount(0) {}

Renderer::~Renderer() {
    if (shaderProgram) glDeleteProgram(shaderProgram);
    if (sphereVAO) glDeleteVertexArrays(1, &sphereVAO);
    if (sphereVBO) glDeleteBuffers(1, &sphereVBO);
    if (window) {
        UserPointer* up = (UserPointer*)glfwGetWindowUserPointer(window);
        delete up;
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

void checkShaderCompile(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
    }
}

void checkProgramLink(GLuint program) {
    GLint success;
    GLchar infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM_LINKING_ERROR\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
    }
}

void Renderer::init(Camera* camera, QuantumState* state) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    window = glfwCreateWindow(width, height, "Atom Prob-Flow", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);

    // Set framebuffer resize callback
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int w, int h) {
        (void)win;
        glViewport(0, 0, w, h);
    });
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }
    
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    setupShaders();
    setupSphere();

    // Setup user pointer for callbacks
    UserPointer* up = new UserPointer{ camera, state, nullptr };
    glfwSetWindowUserPointer(window, up);

    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int mods) {
        UserPointer* actual_up = (UserPointer*)glfwGetWindowUserPointer(win);
        actual_up->camera->processMouseButton(button, action, mods, win);
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y) {
        UserPointer* actual_up = (UserPointer*)glfwGetWindowUserPointer(win);
        actual_up->camera->processMouseMove(x, y);
    });

    glfwSetScrollCallback(window, [](GLFWwindow* win, double xoffset, double yoffset) {
        UserPointer* actual_up = (UserPointer*)glfwGetWindowUserPointer(win);
        actual_up->camera->processScroll(xoffset, yoffset);
    });
}

void Renderer::setParticles(std::vector<Particle>* particles) {
    UserPointer* up = (UserPointer*)glfwGetWindowUserPointer(window);
    if (up) up->particles = particles;
}

void Renderer::setupShaders() {
    const char* vShaderCode = R"glsl(
        #version 330 core
        layout(location=0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        out float lightIntensity;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            vec3 normal = normalize(aPos);
            vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
            lightIntensity = max(dot(normal, lightDir), 0.5);
        } )glsl";

    const char* fShaderCode = R"glsl(
        #version 330 core
        in float lightIntensity;
        out vec4 FragColor;
        uniform vec4 objectColor;
        void main() {
            FragColor = vec4(objectColor.rgb, objectColor.a);
        } )glsl";

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkShaderCompile(vertex, "VERTEX");

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkShaderCompile(fragment, "FRAGMENT");

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);
    checkProgramLink(shaderProgram);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    modelLoc = glGetUniformLocation(shaderProgram, "model");
    viewLoc  = glGetUniformLocation(shaderProgram, "view");
    projLoc  = glGetUniformLocation(shaderProgram, "projection");
    colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
}

void Renderer::setupSphere() {
    std::vector<float> vertices;
    float r = 0.15f;
    int stacks = 10, sectors = 10;
    for(int i = 0; i <= stacks; ++i){
        float t1 = (float)i / stacks * (float)M_PI;
        float t2 = (float)(i+1) / stacks * (float)M_PI;
        for(int j = 0; j < sectors; ++j){
            float p1 = (float)j / sectors * 2.0f * (float)M_PI;
            float p2 = (float)(j+1) / sectors * 2.0f * (float)M_PI;
            auto getPos = [&](float t, float p) {
                return glm::vec3(r*std::sin(t)*std::cos(p), r*std::cos(t), r*std::sin(t)*std::sin(p));
            };
            glm::vec3 v1 = getPos(t1, p1), v2 = getPos(t1, p2), v3 = getPos(t2, p1), v4 = getPos(t2, p2);
            vertices.insert(vertices.end(), {v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z});
            vertices.insert(vertices.end(), {v2.x, v2.y, v2.z, v4.x, v4.y, v4.z, v3.x, v3.y, v3.z});
        }
    }
    sphereVertexCount = (int)vertices.size() / 3;
    createVBOVAO(sphereVAO, sphereVBO, vertices);
}

void Renderer::createVBOVAO(GLuint& VAO, GLuint& VBO, const std::vector<float>& vertices) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void Renderer::createVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(float), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Renderer::drawSpheres(const std::vector<Particle>& particles, const Camera& camera, const QuantumState& state) {
    glUseProgram(shaderProgram);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    float aspect = (fbHeight > 0) ? (float)fbWidth / (float)fbHeight : 1.0f;

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 2000.0f);
    glm::mat4 view = glm::lookAt(camera.position(), camera.target, glm::vec3(0, 1, 0));

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(sphereVAO);

    for (const auto& p : particles) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), p.pos);
        model = glm::scale(model, glm::vec3(state.electron_r));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform4f(colorLoc, p.color.r, p.color.g, p.color.b, p.color.a);

        glDrawArrays(GL_TRIANGLES, 0, sphereVertexCount);
    }
}

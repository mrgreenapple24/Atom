#include "Renderer.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Physics.h"
#include "Font.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Renderer::Renderer() : window(nullptr), shaderProgram(0), textShaderProgram(0), sphereVAO(0), sphereVBO(0), sphereVertexCount(0), textVAO(0), textVBO(0), fontTexture(0) {}

Renderer::~Renderer() {
    if (shaderProgram) glDeleteProgram(shaderProgram);
    if (textShaderProgram) glDeleteProgram(textShaderProgram);
    if (sphereVAO) glDeleteVertexArrays(1, &sphereVAO);
    if (sphereVBO) glDeleteBuffers(1, &sphereVBO);
    if (textVAO) glDeleteVertexArrays(1, &textVAO);
    if (textVBO) glDeleteBuffers(1, &textVBO);
    if (fontTexture) glDeleteTextures(1, &fontTexture);
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
    setupText();

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

    // Text shader
    const char* textVShaderCode = R"glsl(
        #version 330 core
        layout(location=0) in vec4 vertex; // <vec2 pos, vec2 tex>
        out vec2 TexCoords;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
            TexCoords = vertex.zw;
        } )glsl";

    const char* textFShaderCode = R"glsl(
        #version 330 core
        in vec2 TexCoords;
        out vec4 color;
        uniform sampler2D text;
        uniform vec3 textColor;
        void main() {    
            vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
            color = vec4(textColor, 1.0) * sampled;
        } )glsl";

    GLuint textVertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(textVertex, 1, &textVShaderCode, NULL);
    glCompileShader(textVertex);
    checkShaderCompile(textVertex, "TEXT_VERTEX");

    GLuint textFragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(textFragment, 1, &textFShaderCode, NULL);
    glCompileShader(textFragment);
    checkShaderCompile(textFragment, "TEXT_FRAGMENT");

    textShaderProgram = glCreateProgram();
    glAttachShader(textShaderProgram, textVertex);
    glAttachShader(textShaderProgram, textFragment);
    glLinkProgram(textShaderProgram);
    checkProgramLink(textShaderProgram);

    glDeleteShader(textVertex);
    glDeleteShader(textFragment);

    textProjLoc = glGetUniformLocation(textShaderProgram, "projection");
    textColorLoc = glGetUniformLocation(textShaderProgram, "textColor");
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

void Renderer::setupText() {
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Create font texture
    unsigned char font_pixels[128 * 8 * 8];
    for (int char_idx = 0; char_idx < 128; char_idx++) {
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                // Reverse bit order to fix lateral flip (bit 7 is left)
                bool pixel = (font8x8_basic[char_idx][row] >> (7 - col)) & 1;
                font_pixels[char_idx * 64 + row * 8 + col] = pixel ? 255 : 0;
            }
        }
    }

    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 8, 128 * 8, 0, GL_RED, GL_UNSIGNED_BYTE, font_pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void Renderer::drawText(const std::string& text, float x, float y, float scale) {
    glUseProgram(textShaderProgram);
    glUniform3f(textColorLoc, 1.0f, 1.0f, 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glBindVertexArray(textVAO);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glm::mat4 projection = glm::ortho(0.0f, (float)fbWidth, 0.0f, (float)fbHeight);
    glUniformMatrix4fv(textProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    float x_offset = x;
    for (char c : text) {
        float xpos = x_offset;
        float ypos = y;
        float w = 8.0f * scale;
        float h = 8.0f * scale;

        float ty = (float)(c) / 128.0f;
        float th = 1.0f / 128.0f;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, ty },            
            { xpos,     ypos,       0.0f, ty + th },
            { xpos + w, ypos,       1.0f, ty + th },

            { xpos,     ypos + h,   0.0f, ty },
            { xpos + w, ypos,       1.0f, ty + th },
            { xpos + w, ypos + h,   1.0f, ty }           
        };

        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x_offset += w;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
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

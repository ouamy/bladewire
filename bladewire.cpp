#include <glad/glad.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

const unsigned int SCREEN_WIDTH = 1920;
const unsigned int SCREEN_HEIGHT = 1080;

glm::vec3 cameraPos   = { 0.0f, 1.0f, 3.0f };
glm::vec3 cameraFront = { 0.0f, 0.0f, -1.0f };
glm::vec3 cameraUp    = { 0.0f, 1.0f,  0.0f };
float yaw = -90.0f, pitch = 0.0f, fov = 45.0f;
float lastX = SCREEN_WIDTH / 2.0f, lastY = SCREEN_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;
int health = 85, ammo = 12;

void onFramebufferResize(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void onMouseMove(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 1.0f / 45000.0f;
    xoffset *= sensitivity * 180.0f;
    yoffset *= sensitivity * 180.0f;

    yaw   += xoffset;
    pitch += yoffset;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(dir);
}

glm::vec3 velocity(0.0f); // current velocity (x,y,z)
bool onGround = true;
const float gravity = -9.8f;
const float jumpVelocity = 4.0f;

void handleKeyboardInput(GLFWwindow* window) {
    float speed = 5.0f * deltaTime;
    glm::vec3 flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(flatFront, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += flatFront * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= flatFront * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += right * speed;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && onGround) {
        velocity.y = jumpVelocity;
        onGround = false;
    }
}

void updatePhysics() {
    if (!onGround) {
        velocity.y += gravity * deltaTime;
        cameraPos.y += velocity.y * deltaTime;

        if (cameraPos.y <= 1.0f) {
            cameraPos.y = 1.0f;
            velocity.y = 0.0f;
            onGround = true;
        }
    }
}

const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* hudVertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
void main() {
    FragColor = vec4(color, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    return shader;
}

GLuint createShaderProgram(const char* vertexSrc, const char* fragSrc) {
    GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

void drawLine(GLuint shader, glm::vec3 start, glm::vec3 end, glm::vec3 color) {
    float vertices[] = {
        start.x, start.y, start.z,
        end.x, end.y, end.z
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glUniform3fv(glGetUniformLocation(shader, "color"), 1, &color[0]);
    glDrawArrays(GL_LINES, 0, 2);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

void drawQuad(GLuint shader, glm::vec2 pos, glm::vec2 size, glm::vec3 color) {
    float x = pos.x;
    float y = pos.y;
    float w = size.x;
    float h = size.y;

    float vertices[] = {
        x,     y,     0.0f,
        x + w, y,     0.0f,
        x + w, y + h, 0.0f,
        x,     y + h, 0.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glUniform3fv(glGetUniformLocation(shader, "color"), 1, &color[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}

void drawGrid(GLuint shader) {
    glUseProgram(shader);
    for (int i = -10; i <= 10; ++i) {
        drawLine(shader, { (float)i, 0, -10 }, { (float)i, 0, 10 }, { 0, 1, 0 });
        drawLine(shader, { -10, 0, (float)i }, { 10, 0, (float)i }, { 0, 1, 0 });
    }
}

void drawHUD(GLuint hudShader) {
    glUseProgram(hudShader);
    glm::mat4 ortho = glm::ortho(0.0f, (float)SCREEN_WIDTH, 0.0f, (float)SCREEN_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(hudShader, "projection"), 1, GL_FALSE, &ortho[0][0]);

    drawQuad(hudShader, { 50, 50 }, { 200 * (health / 100.0f), 25 }, { 1, 0, 0 });
    drawQuad(hudShader, { SCREEN_WIDTH - 150, 50 }, { 100, 25 }, { 1, 1, 1 });
    drawQuad(hudShader, { SCREEN_WIDTH / 2 - 50, 50 }, { 100, 25 }, { 0, 0, 1 });

    for (int i = 0; i < 5; ++i) {
        drawQuad(hudShader, { 400 + i * 60, SCREEN_HEIGHT - 40 }, { 40, 20 }, { 1, 1, 1 });
        drawQuad(hudShader, { SCREEN_WIDTH - 700 + i * 60, SCREEN_HEIGHT - 40 }, { 40, 20 }, { 1, 0, 0 });
    }
}

void drawWall(GLuint shader, glm::vec3 pos, glm::vec3 size, glm::vec3 color) {
    float x = pos.x;
    float y = pos.y;
    float z = pos.z;
    float w = size.x;
    float h = size.y;
    float d = size.z;

    float vertices[] = {
        // front face
        x,     y,     z + d,
        x + w, y,     z + d,
        x + w, y + h, z + d,
        x,     y + h, z + d,

        // back face
        x,     y,     z,
        x + w, y,     z,
        x + w, y + h, z,
        x,     y + h, z,
    };

    unsigned int indices[] = {
        0,1,2, 2,3,0, // front
        4,5,6, 6,7,4, // back
        0,1,5, 5,4,0, // bottom
        3,2,6, 6,7,3, // top
        1,2,6, 6,5,1, // right
        0,3,7, 7,4,0  // left
    };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glUniform3fv(glGetUniformLocation(shader, "color"), 1, &color[0]);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}

void drawWalls(GLuint shader) {
    float height = 2.0f;
    float thickness = 1.0f;
    float min = -10.0f;
    float max = 10.0f;

    // Front wall (along x axis)
    drawWall(shader, {min, 0.0f, max}, {max - min + thickness, height, thickness}, {1.0f, 1.0f, 1.0f});
    // Back wall
    drawWall(shader, {min, 0.0f, min - thickness}, {max - min + thickness, height, thickness}, {1.0f, 1.0f, 1.0f});
    // Left wall (along z axis)
    drawWall(shader, {min - thickness, 0.0f, min}, {thickness, height, max - min + thickness}, {1.0f, 1.0f, 1.0f});
    // Right wall
    drawWall(shader, {max, 0.0f, min}, {thickness, height, max - min + thickness}, {1.0f, 1.0f, 1.0f});
}

void handleCollision() {
    float margin = 0.5f;
    float min = -10.0f + margin;
    float max = 10.0f - margin;

    if (cameraPos.x < min) cameraPos.x = min;
    if (cameraPos.x > max) cameraPos.x = max;
    if (cameraPos.z < min) cameraPos.z = min;
    if (cameraPos.z > max) cameraPos.z = max;
}

void drawCrosshair(GLuint hudShader) {
    glUseProgram(hudShader);
    glm::mat4 ortho = glm::ortho(0.0f, (float)SCREEN_WIDTH, 0.0f, (float)SCREEN_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(hudShader, "projection"), 1, GL_FALSE, &ortho[0][0]);

    float centerX = SCREEN_WIDTH / 2.0f;
    float centerY = SCREEN_HEIGHT / 2.0f;
    float size = 10.0f; // length of each crosshair arm

    // Horizontal line vertices
    float horizStart[] = { centerX - size, centerY, 0.0f };
    float horizEnd[]   = { centerX + size, centerY, 0.0f };

    // Vertical line vertices
    float vertStart[] = { centerX, centerY - size, 0.0f };
    float vertEnd[]   = { centerX, centerY + size, 0.0f };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Draw horizontal line
    float horizVertices[] = {
        horizStart[0], horizStart[1], horizStart[2],
        horizEnd[0],   horizEnd[1],   horizEnd[2]
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(horizVertices), horizVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glUniform3f(glGetUniformLocation(hudShader, "color"), 1.0f, 0.0f, 0.0f);
    glDrawArrays(GL_LINES, 0, 2);

    // Draw vertical line
    float vertVertices[] = {
        vertStart[0], vertStart[1], vertStart[2],
        vertEnd[0],   vertEnd[1],   vertEnd[2]
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertVertices), vertVertices, GL_STATIC_DRAW);
    glDrawArrays(GL_LINES, 0, 2);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "BladeWire", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onFramebufferResize);
    glfwSetCursorPosCallback(window, onMouseMove);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint shader3D = createShaderProgram(vertexShaderSrc, fragmentShaderSrc);
    GLuint shaderHUD = createShaderProgram(hudVertexShaderSrc, fragmentShaderSrc);

    while (!glfwWindowShouldClose(window)) {
        float current = glfwGetTime();
        deltaTime = current - lastFrame;
        lastFrame = current;

        handleKeyboardInput(window);
        updatePhysics();
        handleCollision();

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader3D);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 proj = glm::perspective(glm::radians(fov), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader3D, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shader3D, "projection"), 1, GL_FALSE, &proj[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shader3D, "model"), 1, GL_FALSE, &model[0][0]);
        drawGrid(shader3D);
        drawWalls(shader3D);

        drawHUD(shaderHUD);
        drawCrosshair(shaderHUD);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

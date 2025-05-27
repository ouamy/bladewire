#include <glad/glad.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>
#include <fstream>
#include <cstring>

const unsigned int SCREEN_WIDTH = 1920;
const unsigned int SCREEN_HEIGHT = 1080;
bool audioInitialized = false;
ALuint source = 0;

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

glm::vec3 velocity(0.0f);
bool onGround = true;
const float gravity = -9.8f;
const float jumpVelocity = 4.0f;

void handleKeyboardInput(GLFWwindow* window) {
    float walkSpeed = 2.5f;
    float runSpeed = 5.0f;
    float speed = walkSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        speed = runSpeed * deltaTime;
    }

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

bool isWalking = false;

void updateWalkingSound(GLFWwindow* window) {
    if (!audioInitialized) return;

    bool moving =
        glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);

    if (moving) {
        bool running = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        float pitch = running ? 1.25f : 1.0f;

        alSourcef(source, AL_PITCH, pitch);

        if (state != AL_PLAYING) {
            alSourcePlay(source);
            isWalking = true;
        }
    } else if (state == AL_PLAYING) {
        alSourcePause(source);
        isWalking = false;
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

    float cx = SCREEN_WIDTH / 2.0f;
    float cy = SCREEN_HEIGHT / 2.0f;
    float thickness = 4.0f;
    float length = 10.0f;

    drawQuad(hudShader, { cx - length, cy - thickness / 2 }, { 2 * length, thickness }, { 1, 0, 0 });
    drawQuad(hudShader, { cx - thickness / 2, cy - length }, { thickness, 2 * length }, { 1, 0, 0 });
}

void drawWall(GLuint shader, glm::vec3 pos, glm::vec3 size, glm::vec3 color) {
    float x = pos.x;
    float y = pos.y;
    float z = pos.z;
    float w = size.x;
    float h = size.y;
    float d = size.z;

    float vertices[] = {

        x,     y,     z + d,
        x + w, y,     z + d,
        x + w, y + h, z + d,
        x,     y + h, z + d,

        x,     y,     z,
        x + w, y,     z,
        x + w, y + h, z,
        x,     y + h, z,
    };

    unsigned int indices[] = {
        0,1,2, 2,3,0,
        4,5,6, 6,7,4,
        0,1,5, 5,4,0,
        3,2,6, 6,7,3,
        1,2,6, 6,5,1,
        0,3,7, 7,4,0
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

    drawWall(shader, {min, 0.0f, max}, {max - min + thickness, height, thickness}, {1.0f, 1.0f, 1.0f});
    drawWall(shader, {min, 0.0f, min - thickness}, {max - min + thickness, height, thickness}, {1.0f, 1.0f, 1.0f});
    drawWall(shader, {min - thickness, 0.0f, min}, {thickness, height, max - min + thickness}, {1.0f, 1.0f, 1.0f});
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

struct WAVData {
    ALsizei size;
    ALsizei freq;
    std::vector<char> data;
    ALenum format;
};

WAVData loadWAV(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    WAVData wav{};

    if (!file) throw std::runtime_error("Failed to open WAV file");

    char chunkId[4];
    uint32_t chunkSize;
    char format[4];

    file.read(chunkId, 4);
    file.read(reinterpret_cast<char*>(&chunkSize), 4);
    file.read(format, 4);

    if (std::strncmp(chunkId, "RIFF", 4) != 0 || std::strncmp(format, "WAVE", 4) != 0)
        throw std::runtime_error("Not a valid WAV file");

    while (file) {
        char subchunkId[4];
        uint32_t subchunkSize;
        file.read(subchunkId, 4);
        file.read(reinterpret_cast<char*>(&subchunkSize), 4);

        if (std::strncmp(subchunkId, "fmt ", 4) == 0) {
            uint16_t audioFormat, numChannels;
            uint32_t sampleRate, byteRate;
            uint16_t blockAlign, bitsPerSample;

            file.read(reinterpret_cast<char*>(&audioFormat), 2);
            file.read(reinterpret_cast<char*>(&numChannels), 2);
            file.read(reinterpret_cast<char*>(&sampleRate), 4);
            file.read(reinterpret_cast<char*>(&byteRate), 4);
            file.read(reinterpret_cast<char*>(&blockAlign), 2);
            file.read(reinterpret_cast<char*>(&bitsPerSample), 2);

            file.seekg(subchunkSize - 16, std::ios_base::cur);

            if (audioFormat != 1) throw std::runtime_error("Only PCM WAV supported");

            if (numChannels == 1) {
                wav.format = (bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
            } else if (numChannels == 2) {
                wav.format = (bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
            } else {
                throw std::runtime_error("Unsupported channel count");
            }

            wav.freq = sampleRate;
        } else if (std::strncmp(subchunkId, "data", 4) == 0) {
            wav.data.resize(subchunkSize);
            file.read(wav.data.data(), subchunkSize);
            wav.size = subchunkSize;
            break;
        } else {
            file.seekg(subchunkSize, std::ios_base::cur);
        }
    }

    return wav;
}

ALCdevice* device = nullptr;
ALCcontext* context = nullptr;
ALuint buffer = 0;

bool initOpenAL(const char* wavFilePath) {
    device = alcOpenDevice(nullptr);
    if (!device) return false;

    context = alcCreateContext(device, nullptr);
    if (!context) {
        alcCloseDevice(device);
        return false;
    }
    alcMakeContextCurrent(context);

    alGenSources(1, &source);
    alGenBuffers(1, &buffer);

    auto loadWav = [](const char* filename, ALuint* outBuffer) -> bool {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return false;

        char riff[4];
        file.read(riff, 4);
        if (std::strncmp(riff, "RIFF", 4) != 0) return false;

        file.seekg(22);
        short channels;
        file.read(reinterpret_cast<char*>(&channels), sizeof(short));
        unsigned int sampleRate;
        file.read(reinterpret_cast<char*>(&sampleRate), sizeof(unsigned int));
        file.seekg(34);
        short bitsPerSample;
        file.read(reinterpret_cast<char*>(&bitsPerSample), sizeof(short));

        char dataChunkId[4];
        unsigned int dataSize = 0;
        while (file.read(dataChunkId, 4)) {
            unsigned int chunkSize = 0;
            file.read(reinterpret_cast<char*>(&chunkSize), sizeof(unsigned int));
            if (std::strncmp(dataChunkId, "data", 4) == 0) {
                dataSize = chunkSize;
                break;
            }
            file.seekg(chunkSize, std::ios::cur);
        }
        if (dataSize == 0) return false;

        std::vector<char> data(dataSize);
        file.read(data.data(), dataSize);

        ALenum format = 0;
        if (channels == 1) {
            format = (bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
        } else if (channels == 2) {
            format = (bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
        } else {
            return false;
        }

        alBufferData(*outBuffer, format, data.data(), static_cast<ALsizei>(data.size()), sampleRate);
        return true;
    };

    if (!loadWav(wavFilePath, &buffer)) return false;

    alSourcei(source, AL_BUFFER, buffer);
    alSourcei(source, AL_LOOPING, AL_TRUE);
    alSourcef(source, AL_GAIN, 0.3f);

    audioInitialized = true;
    return true;
}

void cleanupOpenAL() {
    if (!audioInitialized) return;

    alSourceStop(source);
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);

    audioInitialized = false;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "BladeWire", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onFramebufferResize);
    glfwSetCursorPosCallback(window, onMouseMove);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    GLuint shaderProgram = createShaderProgram(vertexShaderSrc, fragmentShaderSrc);
    GLuint hudShader = createShaderProgram(hudVertexShaderSrc, fragmentShaderSrc);

    glEnable(GL_DEPTH_TEST);

    if (!initOpenAL("sounds/walking.wav")) {
        std::cerr << "Failed to initialize audio\n";
    } 

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        handleKeyboardInput(window);
        handleCollision();
        updatePhysics();
        updateWalkingSound(window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);

        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);

        drawGrid(shaderProgram);
        drawWalls(shaderProgram);

        drawHUD(hudShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    cleanupOpenAL();
    glfwTerminate();
    return 0;
}

#include "renderer.hpp"
#include "shader.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Renderer::Renderer(unsigned int width, unsigned int height, std::shared_ptr<GameController> ctrl)
    : controller(ctrl)
    , screenWidth(width)
    , screenHeight(height)
{
}

Renderer::~Renderer() {}

bool Renderer::initialize() {
    // Create shader programs using the improved shader sources
    shaderProgram = createShaderProgram(improvedVertexShaderSrc, improvedFragmentShaderSrc);
    hudShader = createShaderProgram(improvedHudVertexShaderSrc, improvedFragmentShaderSrc);
    
    if (!shaderProgram || !hudShader) {
        std::cerr << "Error: Could not create shader programs" << std::endl;
        return false;
    }

    // Disable face culling to see both sides
    glDisable(GL_CULL_FACE);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Load both static and animated models
    try {
        // Load the static model
        staticModel = std::make_unique<Model>("view/resources/skins/men/yahya/everyday.fbx");
        
        // Load the animated model
        model = std::make_unique<Model>("view/resources/skins/men/yahya/animations/everyday_idle.fbx");
        
        // Initialize text rendering
        initialiseGLText();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error during initialization: " << e.what() << std::endl;
        return false;
    }
}

void Renderer::render(GLFWwindow* window) {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Update animation if present
    if (model && model->hasAnimation()) {
        model->update(controller->getDeltaTime());
    }
    
    // Create view and projection matrices
    glm::mat4 view = glm::lookAt(
        controller->getCameraPos(),
        controller->getCameraPos() + controller->getCameraFront(),
        controller->getCameraUp()
    );
    
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)screenWidth / (float)screenHeight,
        0.1f,
        100.0f
    );
    
    // Set shader uniforms
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    // Draw the animated model
    if (model) {
        glm::mat4 characterModelMat = glm::mat4(1.0f);
        glm::vec3 playerPos = controller->getCameraPos();
        glm::vec3 offset(0.0f, -1.01f, 3.0f);
        characterModelMat = glm::translate(glm::mat4(1.0f), playerPos + offset);
        
        float yawCorrection = 90.0f;
        characterModelMat = glm::rotate(characterModelMat, glm::radians(-controller->getYaw() + yawCorrection), glm::vec3(0, 1, 0));
        characterModelMat = glm::rotate(characterModelMat, glm::radians(0.0f), glm::vec3(1, 0, 0));
        characterModelMat = glm::scale(characterModelMat, glm::vec3(0.006f));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &characterModelMat[0][0]);
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);
        model->draw(shaderProgram);
    }
    
    // Draw the static model (offset to the side)
    if (staticModel) {
        glm::mat4 staticModelMat = glm::mat4(1.0f);
        glm::vec3 playerPos = controller->getCameraPos();
        glm::vec3 offset(2.0f, -1.01f, 3.0f); // Offset to the right
        staticModelMat = glm::translate(glm::mat4(1.0f), playerPos + offset);
        
        float yawCorrection = 90.0f;
        staticModelMat = glm::rotate(staticModelMat, glm::radians(-controller->getYaw() + yawCorrection), glm::vec3(0, 1, 0));
        staticModelMat = glm::rotate(staticModelMat, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        staticModelMat = glm::scale(staticModelMat, glm::vec3(0.6f));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &staticModelMat[0][0]);
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);
        staticModel->draw(shaderProgram);
    }
    
    // Draw environment
    glm::mat4 platformModelMat = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &platformModelMat[0][0]);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 1.0f, 1.0f);

    drawGrid();
    drawWalls();
    drawHUD(window);
}

void Renderer::drawLine(GLuint shader, glm::vec3 start, glm::vec3 end, glm::vec3 color) {
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

void Renderer::drawQuad(GLuint shader, glm::vec2 pos, glm::vec2 size, glm::vec3 color) {
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

void Renderer::drawGrid() {
    glUseProgram(shaderProgram);
    for (int i = -10; i <= 10; ++i) {
        drawLine(shaderProgram, { (float)i, 0, -10 }, { (float)i, 0, 10 }, { 0, 1, 0 });
        drawLine(shaderProgram, { -10, 0, (float)i }, { 10, 0, (float)i }, { 0, 1, 0 });
    }
}

void Renderer::drawHUD(GLFWwindow* window) {
    glUseProgram(hudShader);
    glm::mat4 ortho = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
    glUniformMatrix4fv(glGetUniformLocation(hudShader, "projection"), 1, GL_FALSE, &ortho[0][0]);

    // Teams
    for (int i = 0; i < 5; ++i) {
        drawQuad(hudShader, { 400 + i * 60, screenHeight - 40 }, { 40, 20 }, { 1, 1, 1 });
        drawQuad(hudShader, { screenWidth - 700 + i * 60, screenHeight - 40 }, { 40, 20 }, { 1, 0, 0 });
    }

    // Crosshair
    float cx = screenWidth / 2.0f;
    float cy = screenHeight / 2.0f;
    float thickness = 4.0f;
    float length = 10.0f;

    drawQuad(hudShader, { cx - length, cy - thickness / 2 }, { 2 * length, thickness }, { 1, 0, 0 });
    drawQuad(hudShader, { cx - thickness / 2, cy - length }, { thickness, 2 * length }, { 1, 0, 0 });
    drawText(window);
}

void Renderer::initialiseGLText() {
    if (!gltInit()) {
        std::cerr << "Error: Could not initialize glText" << std::endl;
    }
    glTextLabel = gltCreateText();
    glTextTimer = gltCreateText();
    countdownStartTime = glfwGetTime();
    healthText.reset(gltCreateText());
}

void Renderer::drawText(GLFWwindow* window) {
    glfwGetFramebufferSize(window, &viewportWidth, &viewportHeight);

    constexpr float TIMER_OFFSET_Y = 30.0f;
    constexpr float TIMER_SIZE = 5.0f;

    constexpr float AMMO_OFFSET_X = 30.0f;
    constexpr float AMMO_OFFSET_Y = 30.0f;
    constexpr float AMMO_SIZE = 5.0f;

    gltBeginDraw();

    double currentTime = glfwGetTime();
    double elapsedTime = currentTime - countdownStartTime;
    double remainingTime = countdownDuration - elapsedTime;
    if (remainingTime < 0) remainingTime = 0;

    int minutes = static_cast<int>(remainingTime) / 60;
    int seconds = static_cast<int>(remainingTime) % 60;

    sprintf(timeString, "%02d:%02d", minutes, seconds);
    gltSetText(glTextTimer, timeString);

    gltColor(
        cosf((float)currentTime) * 0.5f + 0.5f,
        sinf((float)currentTime) * 0.5f + 0.5f,
        1.0f,
        1.0f
    );

    gltDrawText2DAligned(
        glTextTimer,
        viewportWidth / 2.0f,
        TIMER_OFFSET_Y,
        TIMER_SIZE,
        GLT_CENTER,
        GLT_TOP
    );

    int ammo = controller->getAmmo();
    int reserve = controller->getReserveAmmo();

    char ammoString[32];
    // ammo
    sprintf(ammoString, "%d/%d", ammo, reserve);
    gltSetText(glTextLabel, ammoString);

    gltColor(1.0f, 1.0f, 1.0f, 1.0f);

    gltDrawText2DAligned(
        glTextLabel,
        viewportWidth - AMMO_OFFSET_X,
        viewportHeight - AMMO_OFFSET_Y,
        AMMO_SIZE,
        GLT_RIGHT,
        GLT_BOTTOM
    );

    // health
    std::string healthStr = std::to_string(controller->getHealth());

    gltSetText(healthText.get(), healthStr.c_str());

    gltColor(1.0f, 1.0f, 1.0f, 1.0f); // White color

    constexpr float HEALTH_X = 1875.0f;
    constexpr float HEALTH_Y = 30.0f;
    constexpr float HEALTH_SIZE = 5.0f;

    gltDrawText2DAligned(
        healthText.get(),
        viewportWidth - HEALTH_X,
        viewportHeight - HEALTH_Y,
        HEALTH_SIZE,
        GLT_LEFT,
        GLT_BOTTOM
    );
}

void Renderer::cleanText() {
    gltDeleteText(glTextLabel);
    gltDeleteText(glTextTimer);
    gltTerminate();
}

void Renderer::drawWall(GLuint shader, glm::vec3 pos, glm::vec3 size, glm::vec3 color) {
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

void Renderer::drawWalls() {
    float height = 2.0f;
    float thickness = 1.0f;
    float min = -10.0f;
    float max = 10.0f;

    drawWall(shaderProgram, {min, 0.0f, max}, {max - min + thickness, height, thickness}, {1.0f, 1.0f, 1.0f});
    drawWall(shaderProgram, {min, 0.0f, min - thickness}, {max - min + thickness, height, thickness}, {1.0f, 1.0f, 1.0f});
    drawWall(shaderProgram, {min - thickness, 0.0f, min}, {thickness, height, max - min + thickness}, {1.0f, 1.0f, 1.0f});
    drawWall(shaderProgram, {max, 0.0f, min}, {thickness, height, max - min + thickness}, {1.0f, 1.0f, 1.0f});
}

void Renderer::loadModel(const std::string& path, bool isAnimated) {
    try {
        if (isAnimated) {
            model = std::make_unique<Model>(path);
        } else {
            staticModel = std::make_unique<Model>(path);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading model: " << e.what() << std::endl;
    }
}


#include "game_controller.hpp"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

GameController::GameController(unsigned int width, unsigned int height)
    : cameraPos(0.0f, 1.0f, 3.0f)
    , cameraFront(0.0f, 0.0f, -1.0f)
    , cameraUp(0.0f, 1.0f, 0.0f)
    , yaw(-90.0f)
    , pitch(0.0f)
    , fov(45.0f)
    , lastX(width / 2.0f)
    , lastY(height / 2.0f)
    , firstMouse(true)
    , velocity(0.0f)
    , onGround(true)
    , gravity(-9.8f)
    , jumpVelocity(4.0f)
    , isWalking(false)
    , screenWidth(width)
    , screenHeight(height)
    , deltaTime(0.0f)
    , lastFrame(0.0f)
    , health(85)
    , ammo(15)
    , reserveAmmo(30)
    , isShooting(false)
    , shootCooldown(0.3f)
    , lastShotTime(0.0f)
{
    audioManager = std::make_shared<AudioManager>();
}

GameController::~GameController() {
    // AudioManager's cleaning handled by destructor
}

bool GameController::initialize() {
    // audio system initialisation
    if (!audioManager->initialize()) {
        std::cerr << "Erreur: Impossible d'initialiser l'audio" << std::endl;
        return false;
    }
    
    if (!audioManager->loadSound("walking", "sounds/walking.wav")) {
        std::cerr << "Erreur: Impossible de charger le son de marche" << std::endl;
        return false;
    }
    
    // load other sounds
    // audioManager->loadSound("jump", "sounds/jump.wav");
    audioManager->loadSound("shoot", "sounds/shoot.wav");
    audioManager->loadSound("reload", "sounds/reload.wav");
    audioManager->loadSound("land", "sounds/land.wav");
    
    return true;
}

void GameController::handleKeyboardInput(GLFWwindow* window) {
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
        
        // usage example
        // audioManager->playSound("jump", false);
    }
}

void GameController::updateWalkingSound(GLFWwindow* window) {
    bool moving =
        glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    if (moving) {
        bool running = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        float pitch = running ? 1.25f : 1.0f;

        // adjusting sound's pitch
        audioManager->setPitch("walking", pitch);

        if (!audioManager->isPlaying("walking")) {
            audioManager->playSound("walking", true); // loop play
            isWalking = true;
        }
    } else if (isWalking) {
        audioManager->pauseSound("walking");
        isWalking = false;
    }
}

void GameController::updatePhysics() {
    bool wasOnGround = onGround;

    if (!onGround) {
        velocity.y += gravity * deltaTime;
        cameraPos.y += velocity.y * deltaTime;

        if (cameraPos.y <= 1.0f) {
            cameraPos.y = 1.0f;
            velocity.y = 0.0f;
            onGround = true;
            audioManager->playSound("land");
        }
    }
}

void GameController::handleCollision() {
    float margin = 0.5f;
    float min = -10.0f + margin;
    float max = 10.0f - margin;

    if (cameraPos.x < min) cameraPos.x = min;
    if (cameraPos.x > max) cameraPos.x = max;
    if (cameraPos.z < min) cameraPos.z = min;
    if (cameraPos.z > max) cameraPos.z = max;
}

void GameController::onMouseMove(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
        return;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;

    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 1.0f / 160.7f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void GameController::onFramebufferResize(GLFWwindow* window, int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void GameController::updateDeltaTime() {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
}

void GameController::handleShooting(GLFWwindow* window) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        float currentTime = glfwGetTime();
        if (!isShooting && currentTime - lastShotTime >= shootCooldown && ammo > 0) {
            isShooting = true;
            lastShotTime = currentTime;
            ammo--;
            audioManager->playSound("shoot");
        }
    } else {
        isShooting = false;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        reload();
    }
}

void GameController::reload() {
    if (reserveAmmo <= 0 || ammo == 15) return;

    int needed = 15 - ammo;
    int toReload = (reserveAmmo >= needed) ? needed : reserveAmmo;

    ammo += toReload;
    reserveAmmo -= toReload;

    audioManager->playSound("reload");
}
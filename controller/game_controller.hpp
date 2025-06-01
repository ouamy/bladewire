#ifndef GAME_CONTROLLER_HPP
#define GAME_CONTROLLER_HPP

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include "../model/audio_manager.hpp"

class GameController {
private:
    std::shared_ptr<AudioManager> audioManager;
    
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    float yaw, pitch, fov;
    float lastX, lastY;
    bool firstMouse;
    
    glm::vec3 velocity;
    bool onGround;
    const float gravity;
    const float jumpVelocity;
    
    bool isWalking;

    bool isShooting;
    float shootCooldown;
    float lastShotTime;
    bool isReloading;
    int ammoToAddAfterReload;
    float reloadEndTime;
    
    unsigned int screenWidth;
    unsigned int screenHeight;
    
    float deltaTime;
    float lastFrame;
    
    int health;
    int ammo;
    int reserveAmmo;

public:
    GameController(unsigned int width, unsigned int height);
    ~GameController();
    
    bool initialize();
    
    void handleKeyboardInput(GLFWwindow* window);
    
    void updateWalkingSound(GLFWwindow* window);
    
    void updatePhysics();
    
    void handleCollision();
    
    void handleShooting(GLFWwindow* window);
    void reload();
    
    void onMouseMove(GLFWwindow* window, double xpos, double ypos);
    void onFramebufferResize(GLFWwindow* window, int width, int height);
    
    const glm::vec3& getCameraPos() const { return cameraPos; }
    const glm::vec3& getCameraFront() const { return cameraFront; }
    const glm::vec3& getCameraUp() const { return cameraUp; }
    
    float getDeltaTime() const { return deltaTime; }
    
    void updateDeltaTime();
    
    int getHealth() const { return health; }
    int getAmmo() const { return ammo; }
    int getReserveAmmo() const { return reserveAmmo; }

    void updateReloading();

    
    std::shared_ptr<AudioManager> getAudioManager() { return audioManager; }
};

#endif // GAME_CONTROLLER_HPP

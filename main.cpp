#include <glad/glad.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

#include "controller/game_controller.hpp"
#include "view/renderer.hpp"

// callbacks GLFW
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* controller = static_cast<GameController*>(glfwGetWindowUserPointer(window));
    controller->onFramebufferResize(window, width, height);
}

void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* controller = static_cast<GameController*>(glfwGetWindowUserPointer(window));
    controller->onMouseMove(window, xpos, ypos);
}

int main() {
    // initialisation GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const unsigned int SCREEN_WIDTH = 1900;
    const unsigned int SCREEN_HEIGHT = 980;

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "BladeWire", nullptr, nullptr);
    if (!window) {
        std::cerr << "Erreur: Impossible de créer la fenêtre GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Erreur: Impossible d'initialiser GLAD" << std::endl;
        return -1;
    }

    auto controller = std::make_shared<GameController>(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!controller->initialize()) {
        std::cerr << "Erreur: Impossible d'initialiser le contrôleur" << std::endl;
        return -1;
    }

    auto renderer = std::make_shared<Renderer>(SCREEN_WIDTH, SCREEN_HEIGHT, controller);
    if (!renderer->initialize()) {
        std::cerr << "Erreur: Impossible d'initialiser le renderer" << std::endl;
        return -1;
    }

    renderer->initialiseGLText();

    glfwSetWindowUserPointer(window, controller.get());
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    // main loop
    while (!glfwWindowShouldClose(window)) {
        controller->updateDeltaTime();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        controller->handleKeyboardInput(window);
        controller->handleShooting(window);
        controller->updatePhysics();
        controller->handleCollision();
        controller->updateWalkingSound(window);

        renderer->render();

        renderer->drawText(window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    renderer->cleanText();

    glfwTerminate();
    return 0;
}
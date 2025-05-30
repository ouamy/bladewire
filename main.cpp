#include <glad/glad.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <cmath>        // [ADDED for cos/sin in gltext demo]
#include <cstdio>       // [ADDED for sprintf in gltext demo]

#define GLT_IMPLEMENTATION // [ADDED: Only once in the entire project]
#include "view/gltext.hpp"      // [ADDED: glText header]

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

    // [ADDED: Initialize glText]
    if (!gltInit()) {
        std::cerr << "Erreur: Impossible d'initialiser glText" << std::endl;
        return -1;
    }

    // [ADDED: Create glText objects]
    GLTtext* glTextLabel = gltCreateText();
    GLTtext* glTextTimer = gltCreateText();
    gltSetText(glTextLabel, "BladeWire");

    int viewportWidth, viewportHeight;
    char timeString[30];

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

        // [ADDED: Get framebuffer size for glText placement]
        glfwGetFramebufferSize(window, &viewportWidth, &viewportHeight);

        // [ADDED: Prepare glText for drawing]
        gltBeginDraw();

        gltColor(1.0f, 1.0f, 1.0f, 1.0f);
        gltDrawText2D(glTextLabel, 20.0f, 20.0f, 1.0f);

        double time = glfwGetTime();
        sprintf(timeString, "Time: %.2f", time);
        gltSetText(glTextTimer, timeString);

        gltColor(
            cosf((float)time) * 0.5f + 0.5f,
            sinf((float)time) * 0.5f + 0.5f,
            1.0f,
            1.0f
        );

        gltDrawText2DAligned(glTextTimer, 0.0f, (GLfloat)viewportHeight, 1.0f, GLT_LEFT, GLT_BOTTOM);
        gltEndDraw();
        // [END ADDED]

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // [ADDED: Cleanup glText]
    gltDeleteText(glTextLabel);
    gltDeleteText(glTextTimer);
    gltTerminate();

    glfwTerminate();
    return 0;
}
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <glad/glad.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include "../controller/game_controller.hpp"
#include <cmath>        // cos/sin in gltext demo
#include <cstdio>       // sprintf in gltext demo
#define GLT_IMPLEMENTATION // only once in the entire project
#include "gltext.hpp"
#include "model.hpp"

class Renderer {
private:
    std::shared_ptr<GameController> controller;
    
    GLuint shaderProgram;
    GLuint hudShader;

    //
    GLTtext* glTextLabel;
    GLTtext* glTextTimer;

    std::unique_ptr<Model> model;

    double countdownStartTime = 0.0;
    const double countdownDuration = 100.0; // 100 seconds countdown
    
    unsigned int screenWidth;
    unsigned int screenHeight;

    //
    char timeString[30];
    int viewportWidth, viewportHeight;
    
    GLuint compileShader(GLenum type, const char* src);
    
    GLuint createShaderProgram(const char* vertexSrc, const char* fragSrc);
    
    void drawLine(GLuint shader, glm::vec3 start, glm::vec3 end, glm::vec3 color);
    void drawQuad(GLuint shader, glm::vec2 pos, glm::vec2 size, glm::vec3 color);
    void drawWall(GLuint shader, glm::vec3 pos, glm::vec3 size, glm::vec3 color);
    
public:
    Renderer(unsigned int width, unsigned int height, std::shared_ptr<GameController> ctrl);
    ~Renderer();
    
    bool initialize();
    
    void render(GLFWwindow* window);
    
    void drawGrid();
    
    void drawHUD(GLFWwindow* window);
    
    void drawWalls();

    void initialiseGLText();

    void drawText(GLFWwindow* window);
    
    void cleanText();
};

#endif // RENDERER_HPP

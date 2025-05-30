#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <glad/glad.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include "../controller/game_controller.hpp"
#include <cmath>        // [ADDED for cos/sin in gltext demo]
#include <cstdio>       // [ADDED for sprintf in gltext demo]
#define GLT_IMPLEMENTATION // [ADDED: Only once in the entire project]
#include "gltext.hpp"      // [ADDED: glText header]

class Renderer {
private:
    std::shared_ptr<GameController> controller;
    
    GLuint shaderProgram;
    GLuint hudShader;
    
    unsigned int screenWidth;
    unsigned int screenHeight;
    
    GLuint compileShader(GLenum type, const char* src);
    
    GLuint createShaderProgram(const char* vertexSrc, const char* fragSrc);
    
    void drawLine(GLuint shader, glm::vec3 start, glm::vec3 end, glm::vec3 color);
    void drawQuad(GLuint shader, glm::vec2 pos, glm::vec2 size, glm::vec3 color);
    void drawWall(GLuint shader, glm::vec3 pos, glm::vec3 size, glm::vec3 color);
    
public:
    Renderer(unsigned int width, unsigned int height, std::shared_ptr<GameController> ctrl);
    ~Renderer();
    
    bool initialize();
    
    void render();
    
    void drawGrid();

    void drawText(const char* text);
    
    void drawHUD();
    
    void drawWalls();

    int initialiseGLText();

    void drawBoth(window, viewportWidth, viewportHeight);

    void cleanBoth(glTextLabel, glTextTimer);
};

#endif // RENDERER_HPP

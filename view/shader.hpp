#pragma once
#include <string>
#include <glad/glad.hpp>
#include <iostream>

// vertex shader that properly handles both static and animated models
const char* improvedVertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool hasAnimation;

// For animation
uniform mat4 finalBonesMatrices[100];

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

void main() {
    TexCoords = aTexCoords;
    
    vec4 totalPosition = vec4(0.0);
    vec3 totalNormal = vec3(0.0);
    
    if (hasAnimation) {
        // Apply bone transformations
        for(int i = 0; i < 4; i++) {
            if(aBoneIDs[i] == -1 || aWeights[i] == 0.0) 
                continue;
                
            // Apply bone transformation
            vec4 localPosition = finalBonesMatrices[aBoneIDs[i]] * vec4(aPos, 1.0);
            totalPosition += localPosition * aWeights[i];
            
            // Transform normal by bone matrix (ignoring translation)
            vec3 localNormal = mat3(finalBonesMatrices[aBoneIDs[i]]) * aNormal;
            totalNormal += localNormal * aWeights[i];
        }
        
        // If no bones affected this vertex, use original position
        if(totalPosition == vec4(0.0)) {
            totalPosition = vec4(aPos, 1.0);
            totalNormal = aNormal;
        }
    } else {
        // No animation, use original position
        totalPosition = vec4(aPos, 1.0);
        totalNormal = aNormal;
    }
    
    // Apply model, view, projection transformations
    gl_Position = projection * view * model * totalPosition;
    
    // Pass transformed normal and fragment position to fragment shader
    Normal = mat3(transpose(inverse(model))) * totalNormal;
    FragPos = vec3(model * totalPosition);
}
)";

const char* improvedFragmentShaderSrc = R"(
#version 330 core
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D diffuseTexture;
uniform vec3 color;
uniform bool useTexture;

void main() {
    if (useTexture)
        FragColor = texture(diffuseTexture, TexCoords);
    else
        FragColor = vec4(color, 1.0);
}
)";

const char* improvedHudVertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(aPos, 1.0);
}
)";

// Utility function to compile shaders
GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation error: " << infoLog << std::endl;
    }
    
    return shader;
}

// Utility function to create shader program
GLuint createShaderProgram(const char* vertexSrc, const char* fragSrc) {
    GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader program linking error: " << infoLog << std::endl;
    }
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}


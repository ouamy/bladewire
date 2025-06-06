#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glad/glad.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "animation/animation.hpp"
#include <memory>
#include <filesystem>
#include <iostream>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    // Animation data
    int boneIDs[4] = {-1, -1, -1, -1};
    float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh {
public:
    Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<Texture>& textures);
    void draw(GLuint shaderProgram);
private:
    GLuint VAO, VBO, EBO;
    std::vector<Texture> textures;
    unsigned int indexCount;
    void setupMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
};

class Model {
public:
    Model(const std::string& path);
    void draw(GLuint shaderProgram);
    void update(float deltaTime);
    bool hasAnimation() const { return isAnimated; }

private:
    std::vector<Mesh> meshes;
    std::string directory;   
    std::vector<Texture> textures_loaded;
    
    // Animation-related members
    std::map<std::string, BoneInfo> boneInfoMap;
    int boneCounter = 0;
    std::vector<Animation> animations;
    std::unique_ptr<Animator> animator;
    const aiScene* scene = nullptr;
    bool isAnimated = false;
    
    // Processing methods
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
    
    // Animation-related methods
    void extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh);
    void loadAnimations(const aiScene* scene);
    void autoPlayAnimation();
    bool detectAnimations(const aiScene* scene);
};

// Utility function for texture loading
unsigned int TextureFromFile(const char* path);


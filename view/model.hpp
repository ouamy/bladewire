#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glad/glad.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

class Mesh {
public:
    Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    void draw(GLuint shaderProgram);

private:
    GLuint VAO, VBO, EBO;
    unsigned int indexCount;

    void setupMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
};

class Model {
public:
    Model(const std::string& path);
    void draw(GLuint shaderProgram);

private:
    std::vector<Mesh> meshes;
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
};

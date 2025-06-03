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

private:
    std::vector<Mesh> meshes;
    std::string directory;   
    std::vector<Texture> textures_loaded;  
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};

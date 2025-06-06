#include "model.hpp"
#include <iostream>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include "resources/stb_image.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<Texture>& textures)
    : textures(textures) {
    setupMesh(vertices, indices);
}

void Mesh::setupMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    indexCount = indices.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // Texture coordinates attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(2);
    
    // Bone IDs attribute (as integers)
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));
    glEnableVertexAttribArray(3);
    
    // Bone weights attribute
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);
}

void Mesh::draw(GLuint shaderProgram) {
    if (!textures.empty()) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0].id);
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);
        glUniform1i(glGetUniformLocation(shaderProgram, "diffuseTexture"), 0);
    } else {
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

Model::Model(const std::string& path) {
    
    size_t lastSlash = path.find_last_of("/\\");
    directory = (lastSlash == std::string::npos) ? "" : path.substr(0, lastSlash);

    Assimp::Importer importer;
    scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_GenSmoothNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return;
    }

    // Process all meshes in the model
    processNode(scene->mRootNode, scene);
    
    // Detect if model has animations
    isAnimated = detectAnimations(scene);
    
    // Load animations if the model is animated
    if (isAnimated) {
        loadAnimations(scene);
        animator = std::make_unique<Animator>();
        
        // Configure bone mapping for the animator
        for (const auto& boneInfo : boneInfoMap) {
            animator->setBoneMapping(boneInfo.first, boneInfo.second.id, boneInfo.second.offset);
        }
        
        // Build node hierarchy for proper bone transformations
        if (scene->mRootNode) {
            animator->buildNodeHierarchy(scene->mRootNode, glm::mat4(1.0f));
        }
        
        // Auto-play the first animation if available
        autoPlayAnimation();
        
    }
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    // Process all meshes in the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    // Process all child nodes recursively
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        
        // Position
        vertex.position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );
        
        // Normal
        if (mesh->HasNormals()) {
            vertex.normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        } else {
            vertex.normal = glm::vec3(0.0f);
        }
        
        // Texture coordinates
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        } else {
            vertex.texCoords = glm::vec2(0.0f);
        }
        
        // Initialize bone data with default values
        for (int j = 0; j < 4; j++) {
            vertex.boneIDs[j] = -1;
            vertex.weights[j] = 0.0f;
        }
        
        vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
    
    // Process bone weights if mesh has bones
    if (mesh->HasBones()) {
        extractBoneWeightForVertices(vertices, mesh);
    }
    
    // Process material textures
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        // Load diffuse textures
        auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
        // Load specular textures
        auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    return Mesh(vertices, indices, textures);
}

void Model::draw(GLuint shaderProgram) {
    // Set animation data in shader if model is animated
    if (isAnimated && animator && animator->isPlaying()) {
        auto transforms = animator->getFinalBoneMatrices();
        for (unsigned int i = 0; i < transforms.size(); i++) {
            std::string name = "finalBonesMatrices[" + std::to_string(i) + "]";
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, GL_FALSE, glm::value_ptr(transforms[i]));
        }
        glUniform1i(glGetUniformLocation(shaderProgram, "hasAnimation"), 1);
    } else {
        glUniform1i(glGetUniformLocation(shaderProgram, "hasAnimation"), 0);
    }

    // Draw all meshes
    for (auto& mesh : meshes) {
        mesh.draw(shaderProgram);
    }
}

void Model::update(float deltaTime) {
    // Update animation if model is animated and has an active animation
    if (isAnimated && animator && animator->isPlaying()) {
        animator->updateAnimation(deltaTime);
    }
}

unsigned int TextureFromFile(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName) {
    std::vector<Texture> textures;
    
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        
        // Check if texture was loaded before
        bool skip = false;
        for (const auto& loadedTex : textures_loaded) {
            if (std::strcmp(loadedTex.path.data(), str.C_Str()) == 0) {
                textures.push_back(loadedTex);
                skip = true;
                break;
            }
        }
        
        if (!skip) {
            Texture texture;
            
            // Try different paths to find the texture
            std::string textureRelativePath = std::string(str.C_Str());
            std::filesystem::path fullPath = std::filesystem::path(directory) / textureRelativePath;
            std::filesystem::path filenamePath = std::filesystem::path(directory) / std::filesystem::path(textureRelativePath).filename();
            
            // Try different locations
            std::vector<std::filesystem::path> candidates = {
                fullPath,
                filenamePath,
                std::filesystem::path(directory) / "textures" / std::filesystem::path(textureRelativePath).filename()
            };
            
            std::filesystem::path finalPath;
            for (const auto& candidate : candidates) {
                if (std::filesystem::exists(candidate)) {
                    finalPath = candidate;
                    break;
                }
            }
            
            // If texture not found, try to find a texture with similar name
            if (finalPath.empty()) {
                std::string filename = std::filesystem::path(textureRelativePath).filename().string();
                std::string filenameBase = filename.substr(0, filename.find_last_of('.'));
                
                for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                    if (entry.is_regular_file()) {
                        std::string entryName = entry.path().filename().string();
                        if (entryName.find(filenameBase) != std::string::npos) {
                            finalPath = entry.path();
                            break;
                        }
                    }
                }
            }
            
            // If texture still not found, use a default texture or continue
            if (finalPath.empty()) {
                std::cerr << "Texture not found: " << textureRelativePath << std::endl;
                continue;
            }
            
            texture.id = TextureFromFile(finalPath.string().c_str());
            texture.type = typeName;
            texture.path = finalPath.string();
            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
    }
    
    return textures;
}

void Model::extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh) {
    // Initialize all vertices with default bone data
    for (auto& vertex : vertices) {
        for (int i = 0; i < 4; i++) {
            vertex.boneIDs[i] = -1;
            vertex.weights[i] = 0.0f;
        }
    }
    
    // Process each bone
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        aiBone* bone = mesh->mBones[boneIndex];
        std::string boneName = bone->mName.C_Str();
        
        // Get or create bone ID
        int boneID = 0;
        if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
            BoneInfo newBoneInfo;
            newBoneInfo.id = boneCounter;
            newBoneInfo.offset = glm::transpose(glm::make_mat4(&bone->mOffsetMatrix.a1));
            boneInfoMap[boneName] = newBoneInfo;
            boneID = boneCounter;
            boneCounter++;
        } else {
            boneID = boneInfoMap[boneName].id;
        }
        
        // Apply weights to vertices
        for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
            unsigned int vertexID = bone->mWeights[weightIndex].mVertexId;
            float weight = bone->mWeights[weightIndex].mWeight;
            
            if (vertexID < vertices.size()) {
                // Find first available slot
                bool added = false;
                for (int i = 0; i < 4; ++i) {
                    if (vertices[vertexID].weights[i] == 0.0f) {
                        vertices[vertexID].boneIDs[i] = boneID;
                        vertices[vertexID].weights[i] = weight;
                        added = true;
                        break;
                    }
                }
                
                // If no slot available, replace the smallest weight
                if (!added) {
                    int minIndex = 0;
                    float minWeight = vertices[vertexID].weights[0];
                    
                    for (int i = 1; i < 4; ++i) {
                        if (vertices[vertexID].weights[i] < minWeight) {
                            minWeight = vertices[vertexID].weights[i];
                            minIndex = i;
                        }
                    }
                    
                    if (weight > minWeight) {
                        vertices[vertexID].boneIDs[minIndex] = boneID;
                        vertices[vertexID].weights[minIndex] = weight;
                    }
                }
            }
        }
    }
    
    // Normalize weights
    for (auto& vertex : vertices) {
        float sum = 0.0f;
        for (int i = 0; i < 4; ++i) {
            sum += vertex.weights[i];
        }
        
        if (sum > 0.0f) {
            for (int i = 0; i < 4; ++i) {
                vertex.weights[i] /= sum;
            }
        }
    }
}

bool Model::detectAnimations(const aiScene* scene) {
    // Check if the model has animations
    if (scene->HasAnimations() && scene->mNumAnimations > 0) {
        return true;
    }
    
    // Check if any mesh has bones
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        if (scene->mMeshes[i]->HasBones() && scene->mMeshes[i]->mNumBones > 0) {
            return true;
        }
    }
    
    return false;
}

void Model::loadAnimations(const aiScene* scene) {
    if (scene->HasAnimations()) {
        for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
            aiAnimation* anim = scene->mAnimations[i];
            std::string animName = anim->mName.length > 0 ? anim->mName.C_Str() : "Animation" + std::to_string(i);
            
            animations.push_back(Animation(scene, animName));
        }
    }
}

void Model::autoPlayAnimation() {
    if (!animations.empty() && animator) {
        animator->playAnimation(&animations[0]);
    } else {
        std::cout << "Cannot start animation: " 
                  << (animations.empty() ? "no animations" : "no animator") << std::endl;
    }
}


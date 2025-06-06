#pragma once
#include <vector>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct BoneInfo {
    int id;
    glm::mat4 offset;
};

struct VertexBoneData {
    unsigned int IDs[4] = {0};
    float Weights[4] = {0.0f};

    void AddBoneData(unsigned int BoneID, float Weight) {
        for (unsigned int i = 0; i < 4; i++) {
            if (Weights[i] == 0.0f) {
                IDs[i] = BoneID;
                Weights[i] = Weight;
                return;
            }
        }
    }
};

struct KeyPosition {
    glm::vec3 position;
    float timeStamp;
};

struct KeyRotation {
    glm::quat orientation;
    float timeStamp;
};

struct KeyScale {
    glm::vec3 scale;
    float timeStamp;
};

struct AnimationNode {
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale> scales;
    std::string name;
    int boneId;

    glm::mat4 getLocalTransform(float animationTime) const;
    int getPositionIndex(float animationTime) const;
    int getRotationIndex(float animationTime) const;
    int getScaleIndex(float animationTime) const;
    float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const;
    glm::vec3 interpolatePosition(float animationTime) const;
    glm::quat interpolateRotation(float animationTime) const;
    glm::vec3 interpolateScaling(float animationTime) const;
};

class Animation {
public:
    Animation() = default;
    Animation(const aiScene* scene, const std::string& animationName);
    
    float getDuration() const { return duration; }
    float getTicksPerSecond() const { return ticksPerSecond; }
    const std::map<std::string, AnimationNode>& getAnimationNodes() const { return animationNodes; }
    const std::string& getName() const { return name; }

private:
    void processAnimation(const aiAnimation* animation);
    void processAnimationNode(const aiNodeAnim* nodeAnim);

    std::string name;
    float duration = 0.0f;
    float ticksPerSecond = 0.0f;
    std::map<std::string, AnimationNode> animationNodes;
};

class Animator {
public:
    Animator();
    void updateAnimation(float dt);
    void playAnimation(const Animation* animation);
    void calculateBoneTransform(const std::string& nodeName, const glm::mat4& parentTransform);
    const std::vector<glm::mat4>& getFinalBoneMatrices() const { return finalBoneMatrices; }
    bool isPlaying() const { return currentAnimation != nullptr; }
    const Animation* getCurrentAnimation() const { return currentAnimation; }
    float getCurrentTime() const { return currentTime; }
    
    // Bone mapping configuration
    void setBoneMapping(const std::string& boneName, int boneId, const glm::mat4& offset);
    
    // Node hierarchy building
    void buildNodeHierarchy(const aiNode* node, const glm::mat4& parentTransform);

private:
    std::vector<glm::mat4> finalBoneMatrices;
    const Animation* currentAnimation;
    float currentTime;
    std::map<std::string, glm::mat4> boneTransforms;
    
    // Bone mapping structures
    std::map<std::string, int> boneIdMap;
    std::map<std::string, glm::mat4> boneOffsets;
    
    // Node hierarchy
    std::map<std::string, std::vector<std::string>> nodeHierarchy;
};


#include "animation.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

Animation::Animation(const aiScene* scene, const std::string& animationName) : name(animationName) {
    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* animation = scene->mAnimations[i];
        if (animation->mName.length > 0) {
            std::string animName = animation->mName.C_Str();
            if (animName == animationName || animationName.empty()) {
                processAnimation(animation);
                break;
            }
        } else if (animationName.empty() && i == 0) {
            // If no name specified, take the first animation
            processAnimation(animation);
            break;
        }
    }
}

void Animation::processAnimation(const aiAnimation* animation) {
    duration = animation->mDuration;
    ticksPerSecond = (animation->mTicksPerSecond != 0) ? animation->mTicksPerSecond : 25.0f;
    
    for (unsigned int i = 0; i < animation->mNumChannels; i++) {
        aiNodeAnim* nodeAnim = animation->mChannels[i];
        processAnimationNode(nodeAnim);
    }
}

void Animation::processAnimationNode(const aiNodeAnim* nodeAnim) {
    AnimationNode node;
    node.name = nodeAnim->mNodeName.C_Str();
    
    // Positions
    for (unsigned int i = 0; i < nodeAnim->mNumPositionKeys; i++) {
        KeyPosition keyPos;
        aiVector3D aiPos = nodeAnim->mPositionKeys[i].mValue;
        keyPos.position = glm::vec3(aiPos.x, aiPos.y, aiPos.z);
        keyPos.timeStamp = nodeAnim->mPositionKeys[i].mTime;
        node.positions.push_back(keyPos);
    }
    
    // Rotations
    for (unsigned int i = 0; i < nodeAnim->mNumRotationKeys; i++) {
        KeyRotation keyRot;
        aiQuaternion aiQuat = nodeAnim->mRotationKeys[i].mValue;
        keyRot.orientation = glm::quat(aiQuat.w, aiQuat.x, aiQuat.y, aiQuat.z);
        keyRot.timeStamp = nodeAnim->mRotationKeys[i].mTime;
        node.rotations.push_back(keyRot);
    }
    
    // Scales
    for (unsigned int i = 0; i < nodeAnim->mNumScalingKeys; i++) {
        KeyScale keyScale;
        aiVector3D aiScale = nodeAnim->mScalingKeys[i].mValue;
        keyScale.scale = glm::vec3(aiScale.x, aiScale.y, aiScale.z);
        keyScale.timeStamp = nodeAnim->mScalingKeys[i].mTime;
        node.scales.push_back(keyScale);
    }
    
    animationNodes[node.name] = node;
}

glm::mat4 AnimationNode::getLocalTransform(float animationTime) const {
    glm::vec3 position = interpolatePosition(animationTime);
    glm::quat rotation = interpolateRotation(animationTime);
    glm::vec3 scale = interpolateScaling(animationTime);
    
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotation_mat = glm::toMat4(rotation);
    glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), scale);
    
    return translation * rotation_mat * scale_mat;
}

int AnimationNode::getPositionIndex(float animationTime) const {
    for (unsigned int i = 0; i < positions.size() - 1; i++) {
        if (animationTime < positions[i + 1].timeStamp)
            return i;
    }
    return positions.size() - 1;
}

int AnimationNode::getRotationIndex(float animationTime) const {
    for (unsigned int i = 0; i < rotations.size() - 1; i++) {
        if (animationTime < rotations[i + 1].timeStamp)
            return i;
    }
    return rotations.size() - 1;
}

int AnimationNode::getScaleIndex(float animationTime) const {
    for (unsigned int i = 0; i < scales.size() - 1; i++) {
        if (animationTime < scales[i + 1].timeStamp)
            return i;
    }
    return scales.size() - 1;
}

float AnimationNode::getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const {
    float scaleFactor = 0.0f;
    float midWayLength = animationTime - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    scaleFactor = midWayLength / framesDiff;
    return scaleFactor;
}

glm::vec3 AnimationNode::interpolatePosition(float animationTime) const {
    if (positions.size() == 1)
        return positions[0].position;
    
    int p0Index = getPositionIndex(animationTime);
    int p1Index = p0Index + 1;
    if (p1Index >= positions.size())
        p1Index = p0Index;
    
    float scaleFactor = getScaleFactor(positions[p0Index].timeStamp, positions[p1Index].timeStamp, animationTime);
    glm::vec3 finalPosition = glm::mix(positions[p0Index].position, positions[p1Index].position, scaleFactor);
    return finalPosition;
}

glm::quat AnimationNode::interpolateRotation(float animationTime) const {
    if (rotations.size() == 1)
        return rotations[0].orientation;
    
    int p0Index = getRotationIndex(animationTime);
    int p1Index = p0Index + 1;
    if (p1Index >= rotations.size())
        p1Index = p0Index;
    
    float scaleFactor = getScaleFactor(rotations[p0Index].timeStamp, rotations[p1Index].timeStamp, animationTime);
    glm::quat finalRotation = glm::slerp(rotations[p0Index].orientation, rotations[p1Index].orientation, scaleFactor);
    finalRotation = glm::normalize(finalRotation);
    return finalRotation;
}

glm::vec3 AnimationNode::interpolateScaling(float animationTime) const {
    if (scales.size() == 1)
        return scales[0].scale;
    
    int p0Index = getScaleIndex(animationTime);
    int p1Index = p0Index + 1;
    if (p1Index >= scales.size())
        p1Index = p0Index;
    
    float scaleFactor = getScaleFactor(scales[p0Index].timeStamp, scales[p1Index].timeStamp, animationTime);
    glm::vec3 finalScale = glm::mix(scales[p0Index].scale, scales[p1Index].scale, scaleFactor);
    return finalScale;
}

Animator::Animator() : currentAnimation(nullptr), currentTime(0.0f) {
    finalBoneMatrices.resize(100, glm::mat4(1.0f));
}

void Animator::updateAnimation(float dt) {
    if (!currentAnimation)
        return;
    
    currentTime += dt * currentAnimation->getTicksPerSecond();
    if (currentTime > currentAnimation->getDuration())
        currentTime = fmod(currentTime, currentAnimation->getDuration());
    
    // Clear previous transformations
    boneTransforms.clear();
    
    // Calculate local transformations for each node
    for (const auto& nodePair : currentAnimation->getAnimationNodes()) {
        const AnimationNode& node = nodePair.second;
        glm::mat4 nodeTransform = node.getLocalTransform(currentTime);
        boneTransforms[node.name] = nodeTransform;
    }
    
    // Calculate global transformations starting from root
    calculateBoneTransform("RootNode", glm::mat4(1.0f));
}

void Animator::playAnimation(const Animation* animation) {
    currentAnimation = animation;
    currentTime = 0.0f;
}

void Animator::setBoneMapping(const std::string& boneName, int boneId, const glm::mat4& offset) {
    boneIdMap[boneName] = boneId;
    boneOffsets[boneName] = offset;
}

void Animator::buildNodeHierarchy(const aiNode* node, const glm::mat4& parentTransform) {
    std::string nodeName = node->mName.C_Str();
    
    // Store parent-child relationships
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        std::string childName = node->mChildren[i]->mName.C_Str();
        nodeHierarchy[nodeName].push_back(childName);
    }
    
    // Process children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        buildNodeHierarchy(node->mChildren[i], parentTransform);
    }
}

void Animator::calculateBoneTransform(const std::string& nodeName, const glm::mat4& parentTransform) {
    glm::mat4 nodeTransform = boneTransforms.count(nodeName) ? boneTransforms[nodeName] : glm::mat4(1.0f);
    glm::mat4 globalTransform = parentTransform * nodeTransform;
    
    // Apply transformation if this is a bone
    if (boneIdMap.find(nodeName) != boneIdMap.end()) {
        int boneId = boneIdMap[nodeName];
        if (boneId >= 0 && boneId < finalBoneMatrices.size()) {
            finalBoneMatrices[boneId] = globalTransform * boneOffsets[nodeName];
        }
    }
    
    // Process children
    if (nodeHierarchy.find(nodeName) != nodeHierarchy.end()) {
        for (const auto& childName : nodeHierarchy[nodeName]) {
            calculateBoneTransform(childName, globalTransform);
        }
    }
}


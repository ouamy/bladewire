#ifndef AUDIO_MANAGER_HPP
#define AUDIO_MANAGER_HPP

#include <AL/al.h>
#include <AL/alc.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

struct WAVData {
    ALsizei size;
    ALsizei freq;
    std::vector<char> data;
    ALenum format;
};

class AudioManager {
private:
    ALCdevice* device;
    ALCcontext* context;
    std::unordered_map<std::string, ALuint> buffers;
    std::unordered_map<std::string, ALuint> sources;
    bool initialized;

    WAVData loadWAV(const std::string& filename);

public:
    AudioManager();
    ~AudioManager();

    bool initialize();
    
    bool loadSound(const std::string& name, const std::string& filePath);
    
    bool playSound(const std::string& name, bool loop = false);
    
    bool pauseSound(const std::string& name);
    
    bool stopSound(const std::string& name);
    
    bool isPlaying(const std::string& name);
    
    bool setPitch(const std::string& name, float pitch);
    
    bool setVolume(const std::string& name, float volume);

    float getSoundDuration(const std::string& soundName);
    
    void cleanup();
};

#endif // AUDIO_MANAGER_HPP

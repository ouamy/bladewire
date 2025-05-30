#include "audio_manager.hpp"
#include <iostream>
#include <fstream>
#include <cstring>

AudioManager::AudioManager() : device(nullptr), context(nullptr), initialized(false) {
}

AudioManager::~AudioManager() {
    cleanup();
}

bool AudioManager::initialize() {
    // OpenAL initialisation
    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "Erreur: Impossible d'ouvrir le périphérique audio" << std::endl;
        return false;
    }

    context = alcCreateContext(device, nullptr);
    if (!context) {
        std::cerr << "Erreur: Impossible de créer le contexte audio" << std::endl;
        alcCloseDevice(device);
        return false;
    }
    
    alcMakeContextCurrent(context);
    initialized = true;
    return true;
}

WAVData AudioManager::loadWAV(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    WAVData wav{};

    if (!file) {
        throw std::runtime_error("Impossible d'ouvrir le fichier WAV: " + filename);
    }

    char chunkId[4];
    uint32_t chunkSize;
    char format[4];

    file.read(chunkId, 4);
    file.read(reinterpret_cast<char*>(&chunkSize), 4);
    file.read(format, 4);

    if (std::strncmp(chunkId, "RIFF", 4) != 0 || std::strncmp(format, "WAVE", 4) != 0) {
        throw std::runtime_error("Format de fichier WAV invalide: " + filename);
    }

    while (file) {
        char subchunkId[4];
        uint32_t subchunkSize;
        file.read(subchunkId, 4);
        file.read(reinterpret_cast<char*>(&subchunkSize), 4);

        if (std::strncmp(subchunkId, "fmt ", 4) == 0) {
            uint16_t audioFormat, numChannels;
            uint32_t sampleRate, byteRate;
            uint16_t blockAlign, bitsPerSample;

            file.read(reinterpret_cast<char*>(&audioFormat), 2);
            file.read(reinterpret_cast<char*>(&numChannels), 2);
            file.read(reinterpret_cast<char*>(&sampleRate), 4);
            file.read(reinterpret_cast<char*>(&byteRate), 4);
            file.read(reinterpret_cast<char*>(&blockAlign), 2);
            file.read(reinterpret_cast<char*>(&bitsPerSample), 2);

            file.seekg(subchunkSize - 16, std::ios_base::cur);

            if (audioFormat != 1) {
                throw std::runtime_error("Seul le format PCM WAV est supporté");
            }

            if (numChannels == 1) {
                wav.format = (bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
            } else if (numChannels == 2) {
                wav.format = (bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
            } else {
                throw std::runtime_error("Nombre de canaux non supporté");
            }

            wav.freq = sampleRate;
        } else if (std::strncmp(subchunkId, "data", 4) == 0) {
            wav.data.resize(subchunkSize);
            file.read(wav.data.data(), subchunkSize);
            wav.size = subchunkSize;
            break;
        } else {
            file.seekg(subchunkSize, std::ios_base::cur);
        }
    }

    return wav;
}

bool AudioManager::loadSound(const std::string& name, const std::string& filePath) {
    if (!initialized) {
        std::cerr << "Erreur: AudioManager non initialisé" << std::endl;
        return false;
    }

    // check if sound already exists
    if (buffers.find(name) != buffers.end()) {
        // sound already loaded
        return true;
    }

    try {
        WAVData wavData = loadWAV(filePath);
        
        ALuint buffer;
        alGenBuffers(1, &buffer);
        
        alBufferData(buffer, wavData.format, wavData.data.data(), wavData.size, wavData.freq);
        
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            std::cerr << "Erreur OpenAL lors du chargement du son: " << error << std::endl;
            alDeleteBuffers(1, &buffer);
            return false;
        }
        
        ALuint source;
        alGenSources(1, &source);
        
        alSourcei(source, AL_BUFFER, buffer);
        
        alSourcef(source, AL_GAIN, 0.3f);
        
        buffers[name] = buffer;
        sources[name] = source;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Erreur lors du chargement du son: " << e.what() << std::endl;
        return false;
    }
}

bool AudioManager::playSound(const std::string& name, bool loop) {
    if (!initialized) {
        std::cerr << "Erreur: AudioManager non initialisé" << std::endl;
        return false;
    }

    auto sourceIt = sources.find(name);
    if (sourceIt == sources.end()) {
        std::cerr << "Erreur: Son '" << name << "' non chargé" << std::endl;
        return false;
    }

    ALuint source = sourceIt->second;
    
    // loop or not
    alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    
    // play sound
    alSourcePlay(source);
    
    return true;
}

bool AudioManager::pauseSound(const std::string& name) {
    if (!initialized) return false;

    auto sourceIt = sources.find(name);
    if (sourceIt == sources.end()) return false;

    alSourcePause(sourceIt->second);
    return true;
}

bool AudioManager::stopSound(const std::string& name) {
    if (!initialized) return false;

    auto sourceIt = sources.find(name);
    if (sourceIt == sources.end()) return false;

    alSourceStop(sourceIt->second);
    return true;
}

bool AudioManager::isPlaying(const std::string& name) {
    if (!initialized) return false;

    auto sourceIt = sources.find(name);
    if (sourceIt == sources.end()) return false;

    ALint state;
    alGetSourcei(sourceIt->second, AL_SOURCE_STATE, &state);
    return (state == AL_PLAYING);
}

bool AudioManager::setPitch(const std::string& name, float pitch) {
    if (!initialized) return false;

    auto sourceIt = sources.find(name);
    if (sourceIt == sources.end()) return false;

    alSourcef(sourceIt->second, AL_PITCH, pitch);
    return true;
}

bool AudioManager::setVolume(const std::string& name, float volume) {
    if (!initialized) return false;

    auto sourceIt = sources.find(name);
    if (sourceIt == sources.end()) return false;

    alSourcef(sourceIt->second, AL_GAIN, volume);
    return true;
}

void AudioManager::cleanup() {
    if (!initialized) return;

    for (auto& source : sources) {
        alSourceStop(source.second);
    }

    for (auto& source : sources) {
        alDeleteSources(1, &source.second);
    }
    sources.clear();

    for (auto& buffer : buffers) {
        alDeleteBuffers(1, &buffer.second);
    }
    buffers.clear();

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
    
    initialized = false;
}

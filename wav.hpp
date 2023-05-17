#ifndef WAV_HPP
#define WAV_HPP

#define BUFFER_LENGTH   4096
#define WAV_HEADER_SIZE 44

#include <Arduino.h>
#include <FS.h>

struct WavHeader {
    uint32_t chunkID;
    uint32_t chunkSize;
    uint32_t format;
    uint32_t subChunk1ID;
    uint32_t subChunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint32_t subChunk2ID;
    uint32_t subChunk2Size;
};

struct Wav8BitLoader {
    WavHeader header;

    Wav8BitLoader(fs::FS &fileSystem, const char *filename);
    ~Wav8BitLoader();
    bool writeData(uint8_t *data, size_t size);
    void printHeader() const;
private:
    bool createHeader(File &file);
    bool loadHeader(File &file);
    
    const char *m_FileName;
    fs::FS &m_FileSystem;
    File m_File;
};

#endif // WAV_HPP
/**
 * Usando a biblioteca mySD disponível em https://github.com/nhatuan84/esp32-micro-sdcard.git.
 * A biblioteca SD padrão do ESP32 não possibilita acesso aleatório aos dados do arquivo.
 */

#ifndef WAV_HPP
#define WAV_HPP

#define BUFFER_LENGTH   64
#define WAV_HEADER_SIZE 44

#include <Arduino.h>
#include <mySD.h>

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

    Wav8BitLoader(char *filename);
    ~Wav8BitLoader();
    bool flush();
    bool writeSample(uint8_t sample);
private:
    bool createHeader(ext::File &file);
    bool loadHeader(ext::File &file);

    uint8_t m_Buffer[BUFFER_LENGTH];
    size_t m_BufferCounter;
    const char *m_FileName;
};

#endif // WAV_HPP
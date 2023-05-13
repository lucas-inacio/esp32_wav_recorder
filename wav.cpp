#include <memory.h>
#include <stdio.h>

#include "wav.hpp"

bool writeLoop(File &file, uint8_t *data, size_t size) {
    size_t bytesRemaining = size;
    size_t bytesWritten = 0;
    size_t offset = 0;
    while(bytesRemaining > 0) {
        bytesWritten = file.write(&data[offset], bytesRemaining);
        bytesRemaining -= bytesWritten;
        offset += bytesWritten;

        if(bytesWritten == 0)
            return false;
    }
    return true;
}

Wav8BitLoader::Wav8BitLoader(fs::FS &fileSystem, const char *filename)
: m_FileName{filename}, m_BufferCounter{0}, m_FileSystem{fileSystem} {
    // Se o arquivo existir abre para leitura e preenche o cabeçalho.
    // Senão cria o cabeçalho do zero. O arquivo é mantido aberto no modo
    // de escrita.
    const char* mode = m_FileSystem.exists(filename) ? FILE_READ : FILE_WRITE;
    File file = m_FileSystem.open(filename, mode);
    bool success;
    if(file) {
        if(file.size() > 0) {
            success = loadHeader(file);
            //  Reabre o arquivo para escrita
            file.close();
            m_File = m_FileSystem.open(filename, FILE_WRITE);
        } else {
            // Força escrita do cabeçalho
            success = createHeader(file);
            file.flush();
            m_File = file;
        }
    }

    if(!success)
        memset(&header, 0, WAV_HEADER_SIZE);
}

Wav8BitLoader::~Wav8BitLoader() {
    if(m_BufferCounter > 0) {
        // Escreve dados restantes
        flush();
        m_File.close();
    }
}

bool Wav8BitLoader::flush() {
    // unsigned long start = millis();

    // Pula para o final do arquivo
    m_File.seek(m_File.size());
    if(writeLoop(m_File, m_Buffer, m_BufferCounter)) {
        // Atualiza os cabeçalhos. Informação de tamanho do arquivo.
        header.chunkSize += m_BufferCounter;
        m_File.seek(4);
        writeLoop(m_File, reinterpret_cast<uint8_t *>(&header.chunkSize), 4);

        header.subChunk2Size += m_BufferCounter;
        m_File.seek(40);
        writeLoop(m_File, reinterpret_cast<uint8_t *>(&header.subChunk2Size), 4);

        m_File.flush();
        m_BufferCounter = 0;
    } else {
        return false;
    }

    // Serial.println(millis() - start);
    return true;
}

bool Wav8BitLoader::writeSample(uint8_t sample) {
    m_Buffer[m_BufferCounter++] = sample;

    // Esvazia o buffer periodicamente
    if(m_BufferCounter == BUFFER_LENGTH) {
        return flush();
    }

    return true;
}

void Wav8BitLoader::printHeader() const {
    Serial.print("RIFF: 0x");
    Serial.println(header.chunkID, HEX);

    Serial.print("Chunk size: ");
    Serial.println(header.chunkSize, DEC);

    Serial.print("Format: 0x");
    Serial.println(header.format, HEX);

    Serial.print("Sub chunk 1 ID: ");
    Serial.println(header.subChunk1ID, HEX);

    Serial.print("Sub chunk 1 size: ");
    Serial.println(header.subChunk1Size, DEC);

    Serial.print("Audio format: ");
    Serial.println(header.audioFormat, DEC);

    Serial.print("Number of channels: ");
    Serial.println(header.numChannels, DEC);

    Serial.print("Sample rate: ");
    Serial.println(header.sampleRate, DEC);

    Serial.print("Byte rate: ");
    Serial.println(header.byteRate, DEC);

    Serial.print("Block align: ");
    Serial.println(header.blockAlign, DEC);

    Serial.print("Bits per sample: ");
    Serial.println(header.bitsPerSample, DEC);

    Serial.print("Sub chunk 2 ID: 0x");
    Serial.println(header.subChunk2ID, HEX);

    Serial.print("Sub chunk 2 size: ");
    Serial.println(header.subChunk2Size, DEC);
}

bool Wav8BitLoader::createHeader(File &file) {
    uint8_t *wavHeader = reinterpret_cast<uint8_t*>(&header);
    // Chunk ID
    wavHeader[0]  = 'R';
    wavHeader[1]  = 'I';
    wavHeader[2]  = 'F';
    wavHeader[3]  = 'F';
    // Chunk Size
    wavHeader[4] = 36; // 36 + Sub chunk 2 size
    wavHeader[5] = 0;
    wavHeader[6] = 0;
    wavHeader[7] = 0;
    // Format
    wavHeader[8]  = 'W';
    wavHeader[9]  = 'A';
    wavHeader[10] = 'V';
    wavHeader[11] = 'E';
    // Sub chunk 1 ID
    wavHeader[12] = 'f';
    wavHeader[13] = 'm';
    wavHeader[14] = 't';
    wavHeader[15] = ' ';
    //  Sub chunk 1 Size
    wavHeader[16] = 16;
    wavHeader[17] = 0;
    wavHeader[18] = 0;
    wavHeader[19] = 0;
    // Audio format PCM
    wavHeader[20] = 1;
    wavHeader[21] = 0;
    // Number of channels 1
    wavHeader[22] = 1;
    wavHeader[23] = 0;
    // Sample rate 8kHz
    wavHeader[24] = 0x40;
    wavHeader[25] = 0x1F;
    wavHeader[26] = 0;
    wavHeader[27] = 0;
    // Byte rate = Sample rate * Num channels * Bytes per sample
    wavHeader[28] = 0x40;
    wavHeader[29] = 0x1F;
    wavHeader[30] = 0;
    wavHeader[31] = 0;
    // Block align = Num channels * Bytes per sample
    wavHeader[32] = 1;
    wavHeader[33] = 0;
    // Bits per sample
    wavHeader[34] = 8;
    wavHeader[35] = 0;
    // Data sub chunk (Sub chunk 2 ID)
    wavHeader[36] = 'd';
    wavHeader[37] = 'a';
    wavHeader[38] = 't';
    wavHeader[39] = 'a';
    // Sub chunk 2 size
    wavHeader[40] = 0;
    wavHeader[41] = 0;
    wavHeader[42] = 0;
    wavHeader[43] = 0;

    return writeLoop(file, wavHeader, WAV_HEADER_SIZE);
}

bool Wav8BitLoader::loadHeader(File &file) {
    uint8_t buffer[WAV_HEADER_SIZE] = { 0 };
    size_t bytesRemaining = WAV_HEADER_SIZE;
    size_t offset = 0;
    while(bytesRemaining > 0) {
        size_t bytesRead = file.readBytes(reinterpret_cast<char*>(&buffer[offset]), bytesRemaining);
        bytesRemaining -= bytesRead;
        offset += bytesRead;

        if(bytesRead == 0) {
            return false;
        }
    }

    memcpy(&header, buffer, WAV_HEADER_SIZE);
    return true;
}
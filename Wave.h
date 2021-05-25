#ifndef WAVE_WRITER_WAVE_H
#define WAVE_WRITER_WAVE_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>

// https://wavefilegem.com/how_wave_files_work.html
struct WaveHeader {
    // First part: RIFF header format
    char riff[4]; // "RIFF" (Little Endian) or "RIFX" (Big Endian)
    unsigned int fileBitCount; // header + raw bits
    char wave[4]; // Always "WAVE"

    // Second part: Wave header format
    char fmt_[4]; // Always "fmt " (there's a space at the end)
    unsigned int formatBitCount; // Usually 16
    unsigned short waveFormat; // Almost always 1 (PCM), 3 if IEEE float
    unsigned int channelCount; // 1 for mono, 2 for stereo
    unsigned int sampleRate; // Samples per second, maximum hertz representable in file
    unsigned int byteRate; // sampleRate * bytesPerSample * channelCount
    unsigned short frameByteCount; // bytesPerSample * channelCount
    unsigned short bitDepth; // bytesPerSample * 8

    // Third part: Non-PCM header (only appears if waveFormat is not PCM)
    unsigned short extensionByteCount; // Almost always 0
    char fact[4]; // Always "fact"
    unsigned int factBitCount; // Almost always 4
    unsigned int frameRate; // Probably the same value as sampleRate

    // Fourth part: Raw data
    char data[4]; // Always "data"
    unsigned int rawBitCount; // amount of bits available to read

    void print() {
        using namespace std;

        cout << "[start, end], byte, name: value" << endl;
        cout << "[0, 3], 4, ChunkID: " << riff[0] << riff[1] << riff[2] << riff[3] << endl;
        cout << "[4, 7], 4, ChunkSize: " << fileBitCount << endl;
        cout << "[8, 11], 4, Format: " << wave[0] << wave[1] << wave[2] << wave[3] << endl;
        cout << "[12, 15], 4, Subchunk1ID: " << fmt_[0] << fmt_[1] << fmt_[2] << fmt_[3] << endl;
        cout << "[16, 19], 4, Subchunk1Size: " << formatBitCount << endl;
        cout << "[20, 21], 2, AudioFormat: " << waveFormat << endl;
        cout << "[22, 23], 2, NumChannels: " << channelCount << endl;
        cout << "[24, 27], 4, SampleRate: " << sampleRate << endl;
        cout << "[28, 31], 4, ByteRate: " << byteRate << endl;
        cout << "[32, 33], 2, BlockAlign: " << frameByteCount << endl;
        cout << "[34, 35], 2, BitsPerSample: " << bitDepth << endl;

        if(waveFormat == 1) {
            cout << "[36, 39], 4, Subchunk2ID: " << data[0] << data[1] << data[2] << data[3] << endl;
            cout << "[40, 43], 4, Subchunk2Size: " << rawBitCount << endl;
        } else {
            cout << "[36, 37], 2, ExtensionSize: " << extensionByteCount << endl;
            cout << "[38, 41], 4, Subchunk2ID: " << fact[0] << fact[1] << fact[2] << fact[3] << endl;
            cout << "[42, 45], 4, Subchunk2Size: " << factBitCount << endl;
            cout << "[46, 49], 4, FrameRate: " << frameRate << endl;
            cout << "[50, 53], 4, Subchunk3ID: " << data[0] << data[1] << data[2] << data[3] << endl;
            cout << "[54, 57], 4, Subchunk3Size: " << rawBitCount << endl;
        }
        cout << endl;
    }

    [[nodiscard]] inline bool isPcm() const {
        return waveFormat == 1;
    }
};

/**
 * A simple wave codec for study proposal
 * T: Example: Use float (for 32 bits float) or short (for signed 16 bits) encoding
 */
template <typename T>
class Wave {
private:
    unsigned int sampleRate{0};
    unsigned short channelCount{0};
    const unsigned short byteDepth{0};

    WaveHeader header{};
    std::vector<T> raw;

    void readRiff(char id[4], std::ifstream& file) {
        std::cout << "Reading riff header" << std::endl;
        memcpy(header.riff, id, 4);

        constexpr short count = 2;
        void* pointers[count] = { &header.fileBitCount, &header.wave };
        short bytes[count] = { 4, 4 };

        for(auto i = 0; i < count; i++) {
            file.read(reinterpret_cast<char *>(pointers[i]), bytes[i]);
        }

        if(memcmp(header.wave, "WAVE", 4) != 0) throw std::runtime_error("Must be a wave file");
    }

    void readFmt(char id[4], std::ifstream& file) {
        std::cout << "Reading fmt header" << std::endl;
        memcpy(header.fmt_, id, 4);

        constexpr short count = 7;
        void* pointers[count] = {
                &header.formatBitCount,
                &header.waveFormat,
                &header.channelCount,
                &header.sampleRate,
                &header.byteRate,
                &header.frameByteCount,
                &header.bitDepth
        };
        short bytes[count] = { 4, 2, 2, 4, 4, 2, 2 };

        for(auto i = 0; i < count; i++) {
            file.read(reinterpret_cast<char *>(pointers[i]), bytes[i]);
        }

        if(!header.isPcm() && header.formatBitCount > 16) {
            file.read(reinterpret_cast<char *>(&header.extensionByteCount), 2);
            // TODO: Add support to extension header
            if(header.extensionByteCount != 0) {
                std::cout << "Skipping extension bytes: " << header.extensionByteCount << " bytes" << std::endl;
                file.ignore(header.extensionByteCount);
            }
        }
    }

    void readData(char id[4], std::ifstream& file) {
        std::cout << "Reading data" << std::endl;
        memcpy(header.data, id, 4);

        file.read(reinterpret_cast<char *>(&header.rawBitCount), 4);

        this->sampleRate = header.sampleRate;
        this->channelCount = header.channelCount;
        auto bytesPerSample = header.bitDepth / 8;

        raw.clear();
        for(int i = 0; i < header.rawBitCount / bytesPerSample; i++) {
            T sample;
            file.read(reinterpret_cast<char *>(&sample), bytesPerSample);
            raw.push_back(sample);
        }
    }

    void readFact(char id[4], std::ifstream& file) {
        std::cout << "Reading fact header" << std::endl;
        memcpy(header.fact, id, 4);

        constexpr short count = 2;
        void* pointers[count] = { &header.factBitCount, &header.frameRate };
        short bytes[count] = { 4, 4 };

        for(auto i = 0; i < count; i++) {
            file.read(reinterpret_cast<char *>(pointers[i]), bytes[i]);
        }

        if(header.factBitCount != 4) throw std::runtime_error("Unsupported fact header");
    }

    void readPeak(char id[4], std::ifstream& file) {
        file.ignore(20);
        std::cout << "Skipping PEAK header" << std::endl;
        // TODO: Add support to PEAK header
    }

    void readUndefinedHeader(char id[4], std::ifstream& file) {
        if(id[0] == 0) return;

        unsigned int headerSize;
        file.read(reinterpret_cast<char *>(&headerSize), 4);
        file.ignore(headerSize);
        std::cout
            << "Unknown header "
            << id[0] << id[0] << id[2] << id[3] <<
            ". Skipping " << headerSize << " bits" << std::endl;
    }
public:
    explicit Wave() = default;

    explicit Wave(unsigned int sampleRate, unsigned int channelCount, unsigned short byteDepth)
        : sampleRate(sampleRate), channelCount(channelCount), byteDepth(byteDepth)
    {

    }

    void read(const std::string& path) {
        using namespace std;

        ifstream file(path, ios::binary);
        if(!file.is_open()) throw runtime_error("File not found");

        char subChunkId[4];
        do {
            file.read(subChunkId, 4);

            // TODO: Add support to RIFX (RIFF is Little Endian, RIFX is Big Endian)

            if(strcmp(subChunkId, "RIFF") == 0) readRiff(subChunkId, file);
            else if(strcmp(subChunkId, "fmt ") == 0) readFmt(subChunkId, file);
            else if(strcmp(subChunkId, "fact") == 0) readFact(subChunkId, file);
            else if(strcmp(subChunkId, "data") == 0) readData(subChunkId, file);
            else if(strcmp(subChunkId, "PEAK") == 0) readPeak(subChunkId, file);
            else readUndefinedHeader(subChunkId, file);
        } while (strcmp(subChunkId, "data") != 0);

        header.print();

        file.close();
    }

    void write(const std::string& path) {
        using namespace std;

        ofstream file(path, std::ios::binary);
        if(!file.is_open()) throw runtime_error("File not found");

        file.write(header.riff, 4);
        file.write(reinterpret_cast<char *>(&header.fileBitCount), 4);
        file.write(header.wave, 4);
        file.write(header.fmt_, 4);
        file.write(reinterpret_cast<char *>(&header.formatBitCount), 4);
        file.write(reinterpret_cast<char *>(&header.waveFormat), 2);
        file.write(reinterpret_cast<char *>(&header.channelCount), 2);
        file.write(reinterpret_cast<char *>(&header.sampleRate), 4);
        file.write(reinterpret_cast<char *>(&header.byteRate), 4);
        file.write(reinterpret_cast<char *>(&header.frameByteCount), 2);
        file.write(reinterpret_cast<char *>(&header.bitDepth), 2);
        file.write(header.data, 4);
        file.write(reinterpret_cast<char *>(&header.rawBitCount), 4);

        header.print();

        for(auto sample: raw) {
            file.write(reinterpret_cast<const char *>(&sample), byteDepth);
        }

        file.close();
    }

    void flush() {
        header.sampleRate = this->sampleRate;
        header.channelCount = this->channelCount;
        header.bitDepth = this->byteDepth * 8;
        header.frameByteCount = this->byteDepth * header.channelCount;
        header.byteRate = header.sampleRate * header.frameByteCount;
        header.rawBitCount = raw.size() * this->byteDepth;
        header.fileBitCount = 44 + header.rawBitCount;
    }
};


#endif //WAVE_WRITER_WAVE_H

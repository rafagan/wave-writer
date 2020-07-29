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
    char riff[4]; // Always "RIFF"
    unsigned int fileBitCount; // header + raw bits
    char wave[4]; // Always "WAVE"
    char fmt_[4]; // Always "fmt " (there's a space at the end)
    unsigned int formatBitCount; // Always 16 (4 previous bytes)

    // Second part: Wave header format
    unsigned short waveFormat; // Almost always 1 (PCM), 3 if IEEE float
    unsigned int channelCount; // 1 for mono, 2 for stereo
    unsigned int sampleRate; // Samples per second, maximum hertz representable in file
    unsigned int byteRate; // sampleRate * bytesPerSample * channelCount
    unsigned short frameByteCount; // bytesPerSample * channelCount
    unsigned short bitDepth; // bytesPerSample * 8

    // Third part: Non-PCM header (only appears if waveFormat is not PCM)
    unsigned short extensionByteCount; // Almost always 0
    char fact[4]; // Always "fact"
    unsigned int factByteCount; // Almost always 4
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


            cout << "[36, 39], 4, Subchunk2ID: " << data[0] << data[1] << data[2] << data[3] << endl;
            cout << "[40, 43], 4, Subchunk2Size: " << rawBitCount << endl;
        }
        cout << endl;
    }
};

/**
 * A simple wave codec for study proposal
 * T: Use float (for 32 bits float) or short (for signed 16 bits) encoding
 */
template <typename T>
class Wave {
private:
    unsigned int sampleRate{0};
    unsigned short channelCount{0};
    const unsigned short byteDepth{0};
    bool decimal{false};

    WaveHeader header{};
    std::vector<T> raw;
public:
    explicit Wave() = default;

    explicit Wave(unsigned int sampleRate, unsigned int channelCount, unsigned short byteDepth, bool decimal)
        : sampleRate(sampleRate), channelCount(channelCount), byteDepth(byteDepth), decimal(decimal)
    {

    }

    void read(const std::string& path) {
        using namespace std;

        ifstream file(path, ios::binary);
        if(!file.is_open()) throw runtime_error("File not found");

        file.read(header.riff, 4);
        file.read(reinterpret_cast<char *>(&header.fileBitCount), 4);
        file.read(header.wave, 4);
        file.read(header.fmt_, 4);
        file.read(reinterpret_cast<char *>(&header.formatBitCount), 4);
        file.read(reinterpret_cast<char *>(&header.waveFormat), 2);
        file.read(reinterpret_cast<char *>(&header.channelCount), 2);
        file.read(reinterpret_cast<char *>(&header.sampleRate), 4);
        file.read(reinterpret_cast<char *>(&header.byteRate), 4);
        file.read(reinterpret_cast<char *>(&header.frameByteCount), 2);
        file.read(reinterpret_cast<char *>(&header.bitDepth), 2);
        file.read(header.data, 4);
        file.read(reinterpret_cast<char *>(&header.rawBitCount), 4);

        header.print();

        this->sampleRate = header.sampleRate;
        this->channelCount = header.channelCount;
        auto bytesPerSample = header.bitDepth / 8;
        constexpr unsigned int maxShort = 1 << 16;

        // Third part: Raw PCM encoded data
        raw.clear();

        for(int i = 0; i < header.rawBitCount / bytesPerSample; i++) {
            T sample;
            file.read(reinterpret_cast<char *>(&sample), bytesPerSample);
            raw.push_back(sample);
        }

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

    void setBinary() {

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

#include <iostream>
#include <cmath>
#include <vector>

#include "Wave.h"

using namespace std;

constexpr char PATH[] = "/Users/rafagan/Desktop/Wave/";

vector<float> genSineData(float secs, unsigned int frequency, unsigned int sampleRate, float amplitude) {
    vector<float> data;
    auto totalSamples = static_cast<unsigned int>(float(sampleRate) * secs);

    for(int i = 0; i < totalSamples; ++i) {
        float phase = (float(i) * float(M_PI) * 2.0f * float(frequency)) / float(sampleRate);
        auto sample = sinf(phase) * amplitude;
        data.push_back(sample);
    }

    return data;
}

int main() {
    auto raw = genSineData(5, 200, 48000, 1.0f);

    auto wave = Wave<short>(48000, 1, 2);
    wave.read(string(PATH) + "ridemsadpcm.wav");
//    wave.flush();
//    wave.write(string(PATH) + "rideOut.wav");
    return 0;
}
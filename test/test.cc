#include <cstdio>
#include "parameters.hh"
#include "grain_oscillator.hh"
#include "dsp.hh"
#include "data.hh"

template<typename T, int NUM_CHANNELS>
class WavWriter {
  FILE* fp;
public:
  WavWriter(const char* filename, int size) {
    fp = fopen(filename, "wb");
    fwrite("RIFF", 4, 1, fp);
    uint32_t len = 44 - 8 + size * NUM_CHANNELS * sizeof(T);
    fwrite(&len, 4, 1, fp);
    fwrite("WAVEfmt ", 8, 1, fp);
    uint32_t bs = 0x10;
    fwrite(&bs, 4, 1, fp);
    uint16_t fmt = 0x1;
    fwrite(&fmt, 2, 1, fp);
    uint16_t chan = NUM_CHANNELS;
    fwrite(&chan, 2, 1, fp);
    uint32_t freq = kSampleRate;
    fwrite(&freq, 4, 1, fp);
    uint32_t bps = kSampleRate * sizeof(T) * NUM_CHANNELS;
    fwrite(&freq, 4, 1, fp);
    uint16_t bpb = sizeof(T) * NUM_CHANNELS;
    fwrite(&bpb, 2, 1, fp);
    uint16_t bpsamples = 8 * sizeof(T);
    fwrite(&bpsamples, 2, 1, fp);
    fwrite("data", 4, 1, fp);
    uint16_t datasize = size * sizeof(T) * NUM_CHANNELS;
    fwrite(&datasize, 4, 1, fp);
  }
  // samples from channels in output are interlaced
  void Write(T *output, int size) {
    fwrite(output, sizeof(T) * NUM_CHANNELS, size, fp);
  }
};

constexpr int kDuration = 10;    // seconds

Wavetable example_buffer{Data::wavetable1.data(), Data::wavetable1.size()-1};

struct Main {
  FilteredGrainOscillator<4, &example_buffer> osc_;
  WavWriter<short, 1> wav_{"test.wav", kDuration * kSampleRate};
  Main() {
    int size = kDuration * kSampleRate;

    Parameters p = {
      .frequency = 0.0015f,
      .formant = 0.15f / example_buffer.size(),
      .amplitude = 1.0f,
      .filters = {.q = 30.0f, .freq = 0.02f, .spread = 1.294f},
    };

    while(size -= kBlockSize) {
      // process by engine
      float r = Random::Float() * 2.0f - 1.0f;
      float temp[kBlockSize];
      osc_.Process(&p, temp, kBlockSize);
      p.formant *= 1.0004f;
      p.frequency *= 0.9995f + r * 0.01f;
      p.amplitude += r * 0.02f;
      p.filters.spread *= 1.00012f;
      p.filters.freq *= 0.9995f;

      // conversion to short
      short output[kBlockSize];
      for(int i=0; i<kBlockSize; i++) {
        output[i] = short_of_float(temp[i]);
      }
      // write
      wav_.Write(output, kBlockSize);
    }
  }
} _;

int main() {}

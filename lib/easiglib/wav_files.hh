
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

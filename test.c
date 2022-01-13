#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <audio/audio_resampler.h>

static inline float read_f32(const uint8_t * p)
{
  struct { float sample; } sample;
  sample.sample = *(float*)(p);
  return (float)(sample.sample);
}

static inline void write_f32(FILE * f, float sample)
{
  fwrite(&sample, 1, 4, f);
}

int main(int argc, char ** argv)
{
  int samples_free;
  size_t read;
  size_t i;
  size_t nf;

  const retro_resampler_t * resampler = NULL;
  void * resampler_data = NULL;

  FILE * f;
  FILE * g = fopen("sweep_out_44.raw", "wb");

  typedef struct sweep_info
  {
    const char * name;
    int freq;
  } sweep_info;

  const sweep_info files[4] = { { "sweep16.raw", 16000 },
                                { "sweep32.raw", 32000 },
                                { "sweep48.raw", 48000 },
                                { "sweep96.raw", 96000 } };

  float buffer[1024];
  float outbuffer[4096];

  float *outptr;

  for ( nf = 0; nf < 4; nf++ )
  {
    f = fopen( files[nf].name, "rb" );

    double ratio = 44100.0 / (double)(files[nf].freq);

    size_t latency;
    size_t dispose_in = 0;
    size_t dispose_out = 0;
    ssize_t prefill_out = -1;
    int next_eof = 0;

    if (!retro_resampler_realloc(&resampler_data, &resampler,
      "sinc", RESAMPLER_QUALITY_HIGHEST, 1, ratio))
      break;

    struct resampler_data src_data;

    src_data.ratio = ratio;

    src_data.data_in = buffer;

    dispose_in = latency = resampler->latency(resampler_data);
    dispose_out = latency * ratio;

    for (;;)
    {
      size_t samples_out;
      size_t samples_skipped = 0;
      size_t samples_in = 1024;
      size_t samples_out_estimated = samples_in * ratio;
      samples_out_estimated = (samples_out_estimated + 7) & ~7;

      if (samples_out_estimated > 4096)
      {
        samples_out_estimated = 4096;
        samples_in = 4096 / ratio;
        if (!samples_in) samples_in = 1;
      }

      {
        uint8_t samples[samples_in * 4];
        if (dispose_in)
        {
          read = dispose_in;
          if (read > samples_in)
            read = samples_in;
          memset(samples, 0, read * 4);
          dispose_in -= read;
          samples_in -= read;
          samples_skipped = read;
        }
        if (!next_eof)
        {
          read = fread(samples + samples_skipped * 4, 4, samples_in, f);
          if (read < samples_in) next_eof = 1;
          read += samples_skipped;
        }
        else
        {
          if (prefill_out < 0)
            prefill_out = latency;
          if (prefill_out)
          {
            read = prefill_out;
            if (read > samples_in)
              read = samples_in;
            memset(samples, 0, read * 4);
            prefill_out -= read;
          }
          else break;
        }
        for (i = 0; i < read; ++i)
        {
          buffer[i] = read_f32(samples + i * 4);
        }
        samples_in = read;
      }

      src_data.input_frames = samples_in;
      src_data.output_frames = 0;
      src_data.data_out = outbuffer;

      resampler->process(resampler_data, &src_data);

      if (dispose_out)
      {
        size_t to_dispose = dispose_out;
        if (to_dispose > src_data.output_frames)
          to_dispose = src_data.output_frames;
        size_t samples_left = src_data.output_frames - to_dispose;
        src_data.data_out += to_dispose;
        src_data.output_frames = samples_left;
        dispose_out -= to_dispose;
      }

      outptr = src_data.data_out;

      for (i = 0, samples_out = src_data.output_frames; i < samples_out; ++i)
      {
        write_f32(g, outptr[i]);
      }

      if (next_eof && !prefill_out)
        break;
    }
    fclose(f);
  }

  fclose(g);

  if (resampler && resampler_data)
    resampler->free(resampler, resampler_data);

  return 0;
}

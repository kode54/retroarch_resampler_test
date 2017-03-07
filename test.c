#include <stdio.h>
#include <stdint.h>

#include "resampler.h"

static inline int32_t read_24(const uint8_t * p)
{
  struct { int32_t sample:24; } sample;
  sample.sample = p[0] + (p[1] * 256) + (p[2] * 65536);
  return sample.sample;
}

static inline void write_24(FILE * f, int32_t sample)
{
  int8_t buffer[3];
  buffer[0] = (sample & 255);
  buffer[1] = (sample >> 8) & 255;
  buffer[2] = (sample >> 16) & 255;
  fwrite(buffer, 1, 3, f);
}

int main(int argc, char ** argv)
{
  int samples_free;
  size_t read;
  size_t i;
  size_t nf;

  void * resampler = resampler_create();

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

  for ( nf = 0; nf < 4; nf++ )
  {
    f = fopen( files[nf].name, "rb" );

    resampler_set_rate(resampler, (double)(files[nf].freq) / 44100.0);

    for (;;)
    {
      samples_free = resampler_get_free(resampler) / 2;
      {
        uint8_t samples[samples_free * 3];
        read = fread(samples, 3, samples_free, f);
        for (i = 0; i < read; ++i)
        {
          resampler_write_sample(resampler, read_24(samples + i * 3));
        }
      }

      while (resampler_get_avail(resampler))
      {
        sample_t sample;
        resampler_read_sample(resampler, &sample);
        if (sample > (1 << 23) - 1)
          sample = (1 << 23) - 1;
        else if (sample < -(1 << 23))
          sample = -(1 << 23);
        write_24(g, sample);
      }

      if (feof(f) && !resampler_get_avail(resampler))
        break;
    }
    fclose(f);
  }

  fclose(g);

  resampler_destroy(resampler);

  return 0;
}

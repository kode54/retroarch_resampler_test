#include <stdio.h>
#include <stdint.h>

#include "resampler.h"

int main(int argc, char ** argv)
{
  int samples_free;
  size_t read;
  size_t i;

  void * resampler = resampler_create();

  FILE * f = fopen("sweep.raw", "rb");
  FILE * g = fopen("sweep44.raw", "wb");

  resampler_set_rate(resampler, 48000.0 / 44100.0);

  for (;;)
  {
    samples_free = resampler_get_free(resampler) / 2;
    {
      int16_t samples[samples_free];
      read = fread(samples, 2, samples_free, f);
      for (i = 0; i < read; ++i)
      {
        resampler_write_pair(resampler, samples[i], samples[i]);
      }
    }

    while (resampler_get_avail(resampler))
    {
      sample_t sample;
      resampler_read_pair(resampler, &sample, &sample);
      fwrite(&sample, sizeof(sample_t), 1, g);
    }

    if (feof(f) && !resampler_get_avail(resampler))
      break;
  }

  fclose(g);
  fclose(f);

  resampler_destroy(resampler);

  return 0;
}

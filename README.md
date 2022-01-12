## RetroArch resampler

# Basic tests

This Makefile and source file generates several test sweeps using Sox,
then processes them with the RetroArch resampler to a single 44100Hz
output file.

The input sweeps generated are linear sine sweeps at -6 dB, from 1Hz
to Nyquist, for the sample rates of 16 kHz, 32 kHz, 48 kHz, and 96 kHz.

The output file generated resamples all of the input files to 44100 Hz,
generating the resampler tables with as much bandwidth is needed for
each file. This is not quite the same thing as the popular SRC comparison.

So, for these results, two of the inputs are being upsampled, and two
of them are being downsampled.

Note that even the "Normal" preset should be fine for most uses, as its
noise floor is quite well below the threshold of hearing.

The lower and lowest presets also have slightly different handling of
the downsampling tasks, as those presets are using non-interpolated
Lanczos windowed filters, while the normal through highest presets
are using an interpolated Kaiser windowed sinc function.


# Results

I have pre-generated the resulting sweeps for all supported quality
levels on an Apple Silicon Mac, with NEON optimizatons enabled.

Lowest quality:
![Lowest quality spectrum](/images/spectrum_retroarch_lowest.png)

Lower quality:
![Lower quality spectrum](/images/spectrum_retroarch_lower.png)

Normal/Dont care quality:
![Normal quality spectrum](/images/spectrum_retroarch_normal.png)

Higher quality:
![Higher quality spectrum](/images/spectrum_retroarch_higher.png)

Highest quality:
![Highest quality spectrum](/images/spectrum_retroarch_highest.png)


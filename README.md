## RetroArch resampler

# Basic tests

This Makefile and source file generates several test sweeps using Sox,
then processes them with the RetroArch resampler to a single 44100Hz
output file.

This version of the RetroArch resampler differs a bit from the official
upstream version. For one thing, I went through the painstaking effort
of implementing arbitrary channel count resampling into all of the
processing functions I included, which is C, SSE2, AVX, and ARM NEON.

I also modified the signed 16 bit integer conversion function to implement
a version for converting signed 32 bit integer samples to float.

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

The general takeaway is that normal or even lower presets are okay
for basic use, as you're not likely to hear the difference in practice.
However, the placebo options are available for those who want them,
and they're certainly still fast.

The code I use in Cog, for instance, supports NEON optimizations on
Apple Silicon, and supports both SSE2 and AVX on Intel machines.
Naturally, AVX is only used for the higher and highest presets, and
only on machines that support AVX. Otherwise, they fall back to SSE2.
This includes on Apple Silicon machines forcibly running the code
under Rosetta 2, which doesn't support AVX.

For extra flair, Cog also uses SSE2 or NEON code to perform signed
integer 16 and 32 bit to float conversions. Signed 24 bit is promoted
to 32 bits first.


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


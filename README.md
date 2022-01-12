## RetroArch resampler

# Basic tests

This Makefile and source file generates several test sweeps using Sox,
then processes them with the RetroArch resampler to a single 44100Hz
output file.

This version of the RetroArch resampler differs a bit from the official
upstream version. For one thing, I went through the painstaking effort
of implementing arbitrary channel count resampling into all of the
processing functions I included, which is C, SSE, AVX, and ARM NEON.

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
Apple Silicon, and supports both SSE and AVX on Intel machines.
Naturally, AVX is only used for the higher and highest presets, and
only on machines that support AVX. Otherwise, they fall back to SSE.
This includes on Apple Silicon machines forcibly running the code
under Rosetta 2, which doesn't support AVX.

For extra flair, Cog also uses SSE2 or NEON code to perform signed
integer 16 and 32 bit to float conversions. Signed 24 bit is promoted
to 32 bits first.


# Resampler presets used

As derived from the original source code, the presets are as follows:

```
Lowest:
         cutoff            = 0.98;
         sidelobes         = 2;
         re->phase_bits    = 12;
         re->subphase_bits = 10;
         window_type       = SINC_WINDOW_LANCZOS;

Lower:
         cutoff            = 0.98;
         sidelobes         = 4;
         re->phase_bits    = 12;
         re->subphase_bits = 10;
         window_type       = SINC_WINDOW_LANCZOS;

Normal / Don't Care:
         cutoff            = 0.825;
         sidelobes         = 8;
         re->phase_bits    = 8;
         re->subphase_bits = 16;
         window_type       = SINC_WINDOW_KAISER;
         re->kaiser_beta   = 5.5;

Higher:
         cutoff            = 0.90;
         sidelobes         = 32;
         re->phase_bits    = 10;
         re->subphase_bits = 14;
         window_type       = SINC_WINDOW_KAISER;
         re->kaiser_beta   = 10.5;
         enable_avx        = 1;

Highest:
         cutoff            = 0.962;
         sidelobes         = 128;
         re->phase_bits    = 10;
         re->subphase_bits = 14;
         window_type       = SINC_WINDOW_KAISER;
         re->kaiser_beta   = 14.5;
         enable_avx        = 1;
```

Cutoff is the frequency cutoff level. If the requested bandwidth ratio is less
than 1.0, then this cutoff is multiplied by that ratio.

Sidelobes is how many zero crossings the filter should have to either side of
the center point. This is doubled to become the base filter taps count. If the
bandwidth ratio is lower than 1.0, then the taps count is then divided by the
ratio, to enlarge the filter for downsampling. Then, the taps count is rounded
up to account for various SIMD instruction sets. If one of the above filters
is using AVX or NEON, then this is rounded up to a multiple of 8 samples. If
neither, then rounded up to a multiple of 4.

Phase bits is the power of two size of the calculated window table, or how
many distinct phases are calculated between each input sample.

Subphase bits is how many extra phase bits of precision are accounted for.
When using a Lanczos window, this extra precision is only used for the
position indicator, and is truncated off when selecting a window. If the
resampler is using a Kaiser window, then this subphase is used to interpolate
between neighboring phases of Kaiser windows.

Window type is either Lanczos, which is not interpolated, or Kaiser, which
generates delta windows for precision linear interpolation between neighboring
phases.

Kaiser beta is a parameter for the Kaiser window generation.

Enable AVX controls whether that quality level will try to use AVX if your
machine supports it. The AVX filter functions are actually slower than the
SSE equivalents for smaller filters, so that is why AVX is only chosen
for the larger filter sizes.


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

Looks pretty scary! But if we go for a more optimistic target...

# Results with -96 dB noise floor target

Lowest quality -96 dB:
![Lowest quality spectrum -96 dB](/images/spectrum_retroarch_96db_lowest.png)

Lower quality -96 dB:
![Lower quality spectrum -96 dB](/images/spectrum_retroarch_96db_lower.png)

Normal/Dont care quality -96 dB:
![Normal quality spectrum -96 dB](/images/spectrum_retroarch_96db_normal.png)

Higher quality -96 dB:
![Higher quality spectrum](/images/spectrum_retroarch_96db_higher.png)

Highest quality -96 dB:
![Highest quality spectrum](/images/spectrum_retroarch_96db_highest.png)

With that being much closer to the usual threshold of human hearing, we
see way more optimistic looking graphs!

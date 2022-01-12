## RetroArch resampler

# Basic tests

This Makefile and source file generates several test sweeps using Sox,
then processes them with the RetroArch resampler to a single 44100Hz
output file.

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


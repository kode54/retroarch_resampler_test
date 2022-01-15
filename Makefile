CFLAGS = -Os -IRetroArch -IRetroArch/libretro-common/include -IVorbis -DHAVE_CONFIG_H=1

OBJS = test.o \
       RetroArch/libretro-common/audio/resampler/drivers/sinc_resampler.o \
       RetroArch/libretro-common/audio/resampler/audio_resampler.o \
       RetroArch/libretro-common/audio/conversion/s16_to_float.o \
       RetroArch/libretro-common/audio/conversion/s32_to_float.o \
       RetroArch/libretro-common/features/features_cpu.o \
       RetroArch/libretro-common/memmap/memalign.o \
       Vorbis/lpc.o

# The following are only needed if the resampler_config struct in audio_resampler.c isn't stubbed out.
# The sinc resampler doesn't even use the structure anyway.
#       RetroArch/libretro-common/encodings/encoding_utf.o \
#       RetroArch/libretro-common/vfs/vfs_implementation.o \
#       RetroArch/libretro-common/lists/string_list.o \
#       RetroArch/libretro-common/file/file_path_io.o \
#       RetroArch/libretro-common/file/file_path.o \
#       RetroArch/libretro-common/file/config_file_userdata.o \
#       RetroArch/libretro-common/file/config_file.o \
#       RetroArch/libretro-common/streams/file_stream.o \
#       RetroArch/libretro-common/time/rtime.o \
#       RetroArch/libretro-common/string/stdstring.o

all: test spectrum.png

test : $(OBJS)
	$(CC) -o $@ $^

.c.o:
	$(CC) -S $(CFLAGS) $*.c -o $*.s
	$(CC) -c $(CFLAGS) $*.s -o $@

sweep16.raw:
	sox -V -r 16000 -c 1 -b 32 -L -e floating-point -n $@ synth 8 sine 1:8000 vol -6dB

sweep32.raw:
	sox -V -r 32000 -c 1 -b 32 -L -e floating-point -n $@ synth 8 sine 1:16000 vol -6dB

sweep48.raw:
	sox -V -r 48000 -c 1 -b 32 -L -e floating-point -n $@ synth 8 sine 1:24000 vol -6dB

sweep96.raw:
	sox -V -r 96000 -c 1 -b 32 -L -e floating-point -n $@ synth 8 sine 1:48000 vol -6dB

sweep_out_44.raw: test sweep16.raw sweep32.raw sweep48.raw sweep96.raw
	./test

spectrum.png: sweep_out_44.raw
	sox -b 32 -L -r 44100 -c 1 -e floating-point $^ -n spectrogram -w Kaiser -z 180 -o $@

clean:
	rm -f $(OBJS) $(patsubst %.o,%.s,$(OBJS)) test sweep{16,32,48,96}.raw sweep_out_44.raw spectrum.png > /dev/null

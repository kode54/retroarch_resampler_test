CFLAGS = -O2

OBJS = test.o resampler.o 

all: test spectrum.png

test : $(OBJS)
	$(CC) -o $@ $^

.c.o:
	$(CC) -c $(CFLAGS) $*.c -o $@

sweep16.raw:
	sox -V -r 16000 -c 1 -b 24 -L -e signed-integer -n $@ synth 8 sine 1:8000 vol -6dB

sweep32.raw:
	sox -V -r 32000 -c 1 -b 24 -L -e signed-integer -n $@ synth 8 sine 1:16000 vol -6dB

sweep48.raw:
	sox -V -r 48000 -c 1 -b 24 -L -e signed-integer -n $@ synth 8 sine 1:24000 vol -6dB

sweep96.raw:
	sox -V -r 96000 -c 1 -b 24 -L -e signed-integer -n $@ synth 8 sine 1:48000 vol -6dB

sweep_out_44.raw: test sweep16.raw sweep32.raw sweep48.raw sweep96.raw
	./test

spectrum.png: sweep_out_44.raw
	sox -b 24 -L -r 44100 -c 1 -e signed-integer $^ -n spectrogram -w Kaiser -z 180 -o $@

clean:
	rm -f $(OBJS) test sweep{16,32,48,96}.raw sweep_out_44.raw spectrum.png > /dev/null


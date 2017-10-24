#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * This program merges a video and audio stream together into a single file
 * for playback on the Nordic NRF52 using our video player. The video stream
 * is a file containing sequential 128x96 resolution frames in 16-bit RGB565
 * pixel format. The audio stream is a file containing a stream of 16-bit
 * audio samples encoded at a rate of 12000Hz, stored 16 bits per sample.
 */

#define LINES_PER_CHUNK		16
#define SAMPLE_RATE		12000

#define FRAME_WIDTH		160
#define FRAME_HEIGHT		120

#define SAMPLES_PER_CHUNK	100

#define VID_BUFSZ		(FRAME_WIDTH * LINES_PER_CHUNK)
#define AUD_BUFSZ		(SAMPLES_PER_CHUNK)

int
main (int argc, char * argv[])
{
	FILE * audio;
	FILE * video;
	FILE * out;
	uint16_t scanline[VID_BUFSZ];
	uint16_t samples[AUD_BUFSZ];
	int i;

printf ("VIDBUF: %d\n", sizeof(scanline));
printf ("AUDBUF: %d\n", sizeof(samples));

	if (argc > 4)
		exit (1);

	video = fopen (argv[1], "r+");

	if (video == NULL) {
		fprintf (stderr, "[%s]: ", argv[1]);
		perror ("file open failed");
		exit (1);
	}

	audio = fopen (argv[2], "r+");

        if (audio == NULL) {
		fprintf (stderr, "[%s]: ", argv[2]);
		perror ("file open failed");
		exit (1);
	}

	out = fopen (argv[3], "w");

	if (out == NULL) {
		fprintf (stderr, "[%s]: ", argv[3]);
		perror ("file open failed");
		exit (1);
	}

	i = 0;
	while (1) {
		if (fread (scanline, sizeof(scanline), 1, video) == 0) {
			printf ("OUT OF VIDEO!!\n");
			break;
		}
		if (fread (samples, sizeof(samples), 1, audio) == 0) {
			printf ("OUT OF AUDIO!!\n");
			break;
		}
		i++;
		fwrite (samples, sizeof(samples), 1, out);
		fwrite (scanline, sizeof(scanline), 1, out);
	}

	printf ("%d PASSES\n", i);
	fclose (video);
	fclose (audio);
	fclose (out);

	exit(0);
}

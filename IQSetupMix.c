// Sample code to set the IQaudio mixer settings correctly for 2vRMS output.
// Also sets GPIO22 to unmute the AMP+ or DigiAMP+ if being used.
//
// G.Garrity Jan 2nd 2016 (C) IQaudio Limited
// Edited 16th Oct 2016 to remove GPIO22 mute settings as this is now handled in the overlay itself.
//
// Compile with gcc IQSetupMix.c -oIQSetupMix -lasound
//



#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include <stdarg.h>

// Needed for Serial number capture
#include <stdlib.h>
#include <string.h>

// Needed for MAC address
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>

// Needed for GPIO Access
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1

// Needed for ALSA mixer settings
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>

// Define DEBUG_PRINT TRUE for output
#define DEBUG_PRINT 0		// 1 debug messages, 0 none
#define TRUE	1
#define FALSE	0

static int
GPIOExport(int pin)
{
#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

static int
GPIOUnexport(int pin)
{
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

static int
GPIODirection(int pin, int dir)
{
	static const char s_directions_str[]  = "in\0out";

#define DIRECTION_MAX 35
	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return(-1);
	}

	if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

static int
GPIORead(int pin)
{
#define VALUE_MAX 30
	char path[VALUE_MAX];
	char value_str[3];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		return(-1);
	}

	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Failed to read value!\n");
		return(-1);
	}

	close(fd);

	return(atoi(value_str));
}

static int
GPIOWrite(int pin, int value)
{
	static const char s_values_str[] = "01";

	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return(-1);
	}

	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

void SetMixerSetting(long volume)
{
    long min, max, currentVolume;
    int x;

    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;

    const char *card = "hw:CARD=IQaudIODAC";
    // Previous linux driver's mixer name
    //   const char *selem_name = "Playback Digital";
    //	const char *selem_name = "PCM";
    const char *selem_name = "Analogue";			// Linux 4.1.6-v7+ #810
    const char *selem_name2 = "Analogue Playback Boost";	// Linux 4.1.6-v7+ #810

    // Setup ALSA access
    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);

    // Make change to Analogue mixer first
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    if (DEBUG_PRINT) printf("Returned card's MIXER range - min: %ld, max: %ld\n", min, max);

    // Get current volume
    if (x = snd_mixer_selem_get_playback_volume (elem, SND_MIXER_SCHN_FRONT_LEFT, &currentVolume))
    {
        if (DEBUG_PRINT) printf("%d %s\n", x, snd_strerror(x));
    }
    else if (DEBUG_PRINT) printf("Current ALSA mixer value: %ld\n", currentVolume);

    // Set value to max
    currentVolume = 1;
    if (x = snd_mixer_selem_set_playback_volume_all (elem, currentVolume))
    {
        if (DEBUG_PRINT) printf("%d %s\n", x, snd_strerror(x));
    }
    else if (DEBUG_PRINT) printf("Current ALSA mixer value: %ld\n", currentVolume);


    // Make changes to Analogue Playback Boost mixer too
    snd_mixer_selem_id_set_name(sid, selem_name2);
    snd_mixer_elem_t* elem2 = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_playback_volume_range(elem2, &min, &max);
    if (DEBUG_PRINT) printf("Returned card's MIXER range - min: %ld, max: %ld\n", min, max);

    // Get current volume
    if (x = snd_mixer_selem_get_playback_volume (elem2, SND_MIXER_SCHN_FRONT_LEFT, &currentVolume))
    {
        if (DEBUG_PRINT) printf("%d %s\n", x, snd_strerror(x));
    }
    else if (DEBUG_PRINT) printf("Current ALSA mixer value: %ld\n", currentVolume);

    // Set value to max
    currentVolume = 1;
    if (x = snd_mixer_selem_set_playback_volume_all (elem2, currentVolume))
    {
        if (DEBUG_PRINT) printf("%d %s\n", x, snd_strerror(x));
    }
    else if (DEBUG_PRINT) printf("Current ALSA mixer value: %ld\n", currentVolume);

    snd_mixer_close(handle);

}

int main(int argc, char * argv[])
{
    printf("IQaudIO Set PCM512x ALSA driver for 2vRMS v1.2 Oct 16th 2016\n\n");
    SetMixerSetting(1);
}


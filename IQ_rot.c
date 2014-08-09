// Rotary Encoder Sample app - IQ_rot.c 
// Makes use of WIRINGPI Library
// USES Raspberry Pi GPIO 23 and 24.
// Adjusts ALSA volume (based on Left channel value) up or down to correspond with rotary encoder direction
// Assumes IQAUDIO.COM Pi-DAC volume range -103dB to 4dB
//
// G.Garrity Aug 9th 2014 IQaudIO.com
//
// Compile with gcc IQ_rot.c -oIQ_rot -lwiringPi -lasound
//


#include <stdio.h>
#include <wiringPi.h>
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>

/*
   Rotary encoder connections:
   Encoder A      - gpio 23   (IQAUDIO.COM PI-DAC 23)
   Encoder B      - gpio 24   (IQAUDIO.COM PI-DAC 24)
   Encoder Common - Pi ground (IQAUDIO.COM PI-DAC GRD)
*/


// Define DEBUG_PRINT TRUE for output
#define DEBUG_PRINT 0

// Define the Raspberry Pi IO Pins being used
#define ENCODER_A 4		// GPIO 23
#define ENCODER_B 5		// GPIO 24

#define TRUE	1
#define FALSE	0

static volatile int encoderPos;
static volatile int lastEncoded;
static volatile int encoded;
static volatile int inCriticalSection = FALSE;

/* forward declaration */
void encoderPulse();


int main(int argc, char * argv[])
{
   int pos = 125;
   long min, mindb, max, maxdb, db_of_i;
   long gpiodelay_value = 50;	

   snd_mixer_t *handle;
   snd_mixer_selem_id_t *sid;

   const char *card = "default";
   const char *selem_name = "Playback Digital";
   int x, mute_state;
   long i, currentdBVolume;

   printf("IQaudIO.com Pi-DAC Volume Control support Rotary Encoder) v1.2 Aug 9th 2014\n\n");

   wiringPiSetup ();

   /* pull up is needed as encoder common is grounded */
   pinMode (ENCODER_A, INPUT);
   pullUpDnControl (ENCODER_A, PUD_UP);
   pinMode (ENCODER_B, INPUT);
   pullUpDnControl (ENCODER_B, PUD_UP);

   encoderPos = pos;

   // Setup ALSA access
   snd_mixer_open(&handle, 0);
   snd_mixer_attach(handle, card);
   snd_mixer_selem_register(handle, NULL, NULL);
   snd_mixer_load(handle);

   snd_mixer_selem_id_alloca(&sid);
   snd_mixer_selem_id_set_index(sid, 0);
   snd_mixer_selem_id_set_name(sid, selem_name);
   snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

   snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
   if (DEBUG_PRINT) printf("Returned card VOLUME range - min: %ld, max: %ld\n", min, max);

   if (x = snd_mixer_selem_ask_playback_vol_dB (elem, (min+1), &db_of_i))
   {
               printf("%d %s\n", x, snd_strerror(x));
   }
   else if (DEBUG_PRINT) printf("min VOLUME value %d = %ld DB\n", min+1 , db_of_i);

   if (x = snd_mixer_selem_ask_playback_vol_dB (elem, (max), &db_of_i))
   {
               printf("%d %s\n", x, snd_strerror(x));
   }
   else if (DEBUG_PRINT) printf("max VOLUME value %d = %ld DB\n", max , db_of_i);

   snd_mixer_selem_get_playback_dB_range(elem, &mindb, &maxdb);
   if (DEBUG_PRINT) printf("Returned card DB Range - mindb: %ld, maxdb: %ld\n", mindb, maxdb);

   // Minimum given is mute, we need the first real value
   min++;


   // Set the midDB to correspond to the actual range
   if (x = snd_mixer_selem_ask_playback_vol_dB (elem, min, &mindb))
   {
        printf("%d %s\n", x, snd_strerror(x));
   }
   else if (DEBUG_PRINT) printf("mindb changed to %ld DB\n", mindb);

   // Get current volume
   if (x = snd_mixer_selem_get_playback_dB (elem, SND_MIXER_SCHN_FRONT_LEFT, &currentdBVolume))
   {
        printf("%d %s\n", x, snd_strerror(x));
   }
   else if (DEBUG_PRINT) printf("Current ALSA volume LEFT: %ld dB\n", currentdBVolume);

   if (x = snd_mixer_selem_get_playback_dB (elem, SND_MIXER_SCHN_FRONT_RIGHT, &currentdBVolume))
   {
        printf("%d %s\n", x, snd_strerror(x));
   }
   else if (DEBUG_PRINT) printf("Current ALSA volume RIGHT: %ld dB\n", currentdBVolume);


   /* monitor encoder level changes */
   wiringPiISR (ENCODER_A, INT_EDGE_BOTH, &encoderPulse);
   wiringPiISR (ENCODER_B, INT_EDGE_BOTH, &encoderPulse);


   // Now sit and spin waiting for GPIO pins to go active...
   while (1)
   {
      if (DEBUG_PRINT) printf("encoderPos:%02x, pos:%02x\n", encoderPos, pos);

      if (encoderPos != pos)
      {
              // get current volume
	      if (x = snd_mixer_selem_get_playback_dB (elem, SND_MIXER_SCHN_FRONT_LEFT, &currentdBVolume))
              {
        		printf("%d %s\n", x, snd_strerror(x));
              }

              // Adjust for MUTE
	      if (currentdBVolume < mindb) currentdBVolume = mindb;

              // What way did the encoder go?
	      if (encoderPos > pos)
	      {
		         pos = encoderPos;
		         if (DEBUG_PRINT) printf("Volume UP %d - ", pos);
			 currentdBVolume = currentdBVolume + 100;
			 // Adjust for MAX volume
			 if (currentdBVolume > maxdb) currentdBVolume = maxdb;
	      }
	      else if (encoderPos < pos)
	      {
		         pos = encoderPos;
		         if (DEBUG_PRINT) printf("Volume DOWN %d - ", pos);
                         currentdBVolume = currentdBVolume - 100;
 
			 // Adjust for MUTE
			 if (currentdBVolume < mindb) currentdBVolume = -99999;
	      }

              if (x = snd_mixer_selem_set_playback_dB_all(elem, currentdBVolume, 0))
              {
                     printf(" ERROR %d %s\n", x, snd_strerror(x));
              } else if (DEBUG_PRINT) printf(" Volume successfully set to %ld dB using ALSA!\n", currentdBVolume);
	}

	// Check x times per second, MAY NEED TO ADJUST THS FREQUENCY FOR SOME ENCODERS */
	delay(gpiodelay_value); /* check pos 50 times per second */
   }

   // We never get here but should close the sockets etc. on exit.
   snd_mixer_close(handle);
}


// Called whenever there is GPIO activity on the defined pins.
void encoderPulse()
{
   /*

             +---------+         +---------+      0
             |         |         |         |
   A         |         |         |         |
             |         |         |         |
   +---------+         +---------+         +----- 1

       +---------+         +---------+            0
       |         |         |         |
   B   |         |         |         |
       |         |         |         |
   ----+         +---------+         +---------+  1

   */

	if (inCriticalSection==TRUE) return;

	inCriticalSection = TRUE;

	int MSB = digitalRead(ENCODER_A);
        int LSB = digitalRead(ENCODER_B);

        int encoded = (MSB << 1) | LSB;
        int sum = (lastEncoded << 2) | encoded;

        if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderPos++;
	else if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderPos--;

	if (DEBUG_PRINT) printf("Got Pulse %02x\n", encoderPos);
        lastEncoded = encoded;

	inCriticalSection = FALSE;
}


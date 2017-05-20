// IR Sample app - IQ_ir.c 
// Makes use of WIRINGPI Library
// USES Raspberry Pi GPIO 25.
//
// Adjusts ALSA volume (based on Left channel value) up or down to correspond with 
// IR inputs for KEY_VOLUMEUP and KEY_VOLUMEDOWN, also takes KEY_MUTE input also
// Assumes IQAUDIO.COM Pi-DAC volume range -103dB to 4dB
//
// G.Garrity May 25th 2015 IQaudIO.com
//
// Compile with 
//	gcc IQ_ir.c -oIQ_ir -lwiringPi -lasound -llirc_client
//
//

#include <stdio.h>
#include <wiringPi.h>
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include <stdbool.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>
/*
   IR Sensor onnections
   IRSensor	  - gpio 25   (IQAUDIO.COM PI-DAC 25)

   Rotary encoder connections:
   Encoder A      - gpio 23   (IQAUDIO.COM PI-DAC 23)
   Encoder B      - gpio 24   (IQAUDIO.COM PI-DAC 24)
   Encoder Common - Pi ground (IQAUDIO.COM PI-DAC GRD)
*/


#define IQTRUE    1
#define IQFALSE   0

// Define DEBUG_PRINT TRUE for output
#define DEBUG_PRINT 0

// Define the Raspberry Pi IO Pins being used for WiringPi
#define IRSENSOR  6 		// GPIO 25

static volatile int buttonTimer;





int main(int argc, char * argv[])
{
   long min, mindb, max, maxdb, db_of_i;
   snd_mixer_t *handle;
   snd_mixer_selem_id_t *sid;

   const char *card = "default";
   const char *selem_name = "Digital";
   int x, mute_state;
   long i, currentVolume;

   // IRSensor data
   struct lirc_config *config;
   int lirc_socket;
   struct lircdata *data;
   int flags;

   //Timer for our buttons
   char *code;
   char *c;
   int IRVALID = IQTRUE;
   int MUTESETTING = IQFALSE;
   int ival;

   printf("IQaudIO.com Pi-DAC Volume Control support (IR) v1.4 March 7th 2016\n\n");

   buttonTimer = 0;

   wiringPiSetup ();
   pinMode (IRSENSOR, INPUT);
   pullUpDnControl (IRSENSOR, PUD_UP);

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

   // Minimum given is mute, we need the first real value
   min++;

   // Get current volume
   if (x = snd_mixer_selem_get_playback_volume (elem, SND_MIXER_SCHN_FRONT_LEFT, &currentVolume))
   {
        printf("%d %s\n", x, snd_strerror(x));
   }
   else if (DEBUG_PRINT) printf("Current ALSA volume LEFT: %ld\n", currentVolume);

   if (x = snd_mixer_selem_get_playback_volume (elem, SND_MIXER_SCHN_FRONT_RIGHT, &currentVolume))
   {
        printf("%d %s\n", x, snd_strerror(x));
   }
   else if (DEBUG_PRINT) printf("Current ALSA volume RIGHT: %ld\n", currentVolume);

   //Initiate LIRC. Exit on failure
   if (lirc_init("lirc",1) !=-1)
   {
	//Read the default LIRC config at /etc/lirc/lircd.conf  This is the config for your remote.
 	if (lirc_readconfig(NULL,&config,NULL) !=0) IRVALID = IQFALSE;

	if (IRVALID)
   	{
		if (DEBUG_PRINT) printf ("IR Sensor active\n");

        	//Do stuff while LIRC socket is open  0=open  -1=closed.
        	while(lirc_nextcode(&code)==0)
        	{
 			//If code = NULL, meaning nothing was returned from LIRC socket,
                	//then skip lines below and start while loop again.
                	if(code==NULL) continue;
			{
				if (DEBUG_PRINT) printf(" Some IR signal received: %s\n",code);

                        	   // Check to see if the string "KEY_MUTE" appears anywhere within the string 'code'.
	                           if(strstr (code,"KEY_PLAYPAUSE"))
				   {
                	                if (DEBUG_PRINT) printf("MATCH on KEY_PLAYPAUSE\n");
                       	                // Call ALSA Here to toggle mute, need to do both LEFT and RIGHT channels
					if (x = snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &ival))
					{
						printf(" ERROR %d %s\n", x, snd_strerror(x));
					}
					if (x = snd_mixer_selem_set_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, (ival ? 1 : 0) ^ 1))
					{
						printf(" ERROR %d %s\n", x, snd_strerror(x));
					}

					// Now the Right Channel too
                                        if (x = snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_RIGHT, &ival))
                                        {
                                                printf(" ERROR %d %s\n", x, snd_strerror(x));
                                        }
                                        if (x = snd_mixer_selem_set_playback_switch(elem, SND_MIXER_SCHN_FRONT_RIGHT, (ival ? 1 : 0) ^ 1))
                                        {
                                                printf(" ERROR %d %s\n", x, snd_strerror(x));
                                        }
					if (DEBUG_PRINT) printf("Toggle Mute - Mute state now %02x\n", ival ? 1 : 0);
					buttonTimer = buttonTimer + 100;
                                   }
                                   else if(strstr (code,"KEY_VOLUMEUP"))
				   {
                                        if (DEBUG_PRINT) printf("MATCH on KEY_VOLUMEUP\n");
                                        // Call ALSA here to UP the volume
		                        if (DEBUG_PRINT) printf("Volume UP\n");

                                        // Get current volume as it could have been changed in parallel outside of this code (alsamixer for example)
					snd_mixer_handle_events(handle); //handle external events such that volume is correct
                                        if (x = snd_mixer_selem_get_playback_volume (elem, SND_MIXER_SCHN_FRONT_LEFT, &currentVolume))
                                        {
                                                printf("%d %s\n", x, snd_strerror(x));
                                        }
                                        else if (DEBUG_PRINT) printf("Volume read for ALSA LEFT: %ld\n", currentVolume);

					// Make sure that we get back from MUTE state
					if (currentVolume < min) currentVolume = min;

                		        currentVolume = currentVolume+10;
                      			// Adjust for MAX volume
                         		if (currentVolume > max) currentVolume = max;
                                        buttonTimer = buttonTimer + 100;
                                   }
                                   else if(strstr (code,"KEY_VOLUMEDOWN"))
				   {
                                        if (DEBUG_PRINT) printf("MATCH on KEY_VOLUMEDOWN\n");
                                        // Call ALSA here to DOWN the volume
		                        if (DEBUG_PRINT) printf("Volume DOWN\n");

                                        // Get current volume as it could have been changed in parallel outside of this code (alsamixer for example)
					snd_mixer_handle_events(handle); //handle external events such that volume is correct
                                        if (x = snd_mixer_selem_get_playback_volume (elem, SND_MIXER_SCHN_FRONT_LEFT, &currentVolume))
                                        {
                                                printf("%d %s\n", x, snd_strerror(x));
                                        }
                                        else if (DEBUG_PRINT) printf("Volume read for ALSA volume LEFT: %ld\n", currentVolume);

                		        currentVolume = currentVolume-10;

                      			// Adjust for MUTE
                         		if (currentVolume < min) currentVolume = 0;
                                        buttonTimer = buttonTimer + 100;
                                   }

				   // Handle MUTE based on MUTESETTING
	              		   if (x = snd_mixer_selem_set_playback_volume_all(elem, currentVolume))
        	      		   {
                	     		printf(" ERROR %d %s\n", x, snd_strerror(x));
              			   } else if (DEBUG_PRINT) printf(" Volume successfully set to %ld using ALSA!\n", currentVolume);
                	} // No IR code received
		} // while
   	} // IRVALID

   } // LIRc active


   // We never get here but should close the sockets etc. on exit.
   snd_mixer_close(handle);

   //Frees the data structures associated with config.
   lirc_freeconfig(config);

   //lirc_deinit() closes the connection to lircd and does some internal clean-up stuff.
   lirc_deinit();

}


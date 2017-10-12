tools
=====

IQaudIO useful tools / sample code etc.

Compilation instructions are in each file's header.

Precompiled versions also included.

## Content

To check that utilities that change volume are working as expected please open another window and launch alsamixer. Check that the mixer control's volume is changing when rotating the rotary encoder or pressing the up/down buttons of your IR handset.

### IQ_rot - Rotary Encoder volume control

Adjusts ALSA volume (based on Left channel value) up or down to correspond with rotary encoder direction.

```
$ sudo IQ_rot &
```

### IQ_ir - IR Volume control Sample app

Adjusts ALSA volume (based on Left channel value) up or down to correspond with IR inputs for `KEY_VOLUMEUP` and `KEY_VOLUMEDOWN`, also takes `KEY_MUTE` input.
- assumes IQAUDIO.COM Pi-DAC volume range -103dB to 4dB
- USES Raspberry Pi GPIO 25

```
$ sudo IQ_ir &
```

### volcontrol.py - Rotary encoder/[reboot/halt] button

Adjust ALSA volume by means of rotary encoder
also incorporates press-switch
-  click = toggle mute
-  hold = shutdown machine

### ButtonPress.py - Reboot/halt button

Key press detect code, uses gpio 27 and ground to a momentary switch. If pressed for more than 1sec but (less than 5) it will force a reboot.
If pressed for more than 5 seconds it will force a shutdown

### IQSetupMix.c - Set 2vRMS output

Sample code to set the IQaudio mixer settings correctly for 2vRMS output.
Also sets GPIO22 to unmute the AMP+ or DigiAMP+ if being used.


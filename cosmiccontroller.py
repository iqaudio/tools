#!/usr/bin/python
#
# $Id: cosmiccontroller.py,v 1.0 2019/10/15 17:53:00 Gordon Exp $
# (C) Copyright IQaudIO Limited, 2017

# Use this code to read from the CosmicController's buttons (1/2/3 and rotary encoder)
# toggle the CosmicController's LEDs (default it at button press)
# and adjust ALSA volume by means of rotary encoder
# NOTE: Rotary encoder's button is time sensitive...
#   click = toggle mute
#
# hardware:
#   IQaudIO CosmicController board v1
#   rotary encoder with common tied to earth
#   momentary switch with one terminal tied to earth
#   all input will use raspberry internal pullups


from time import sleep
from subprocess import call
import datetime
import subprocess
import RPi.GPIO as GPIO

# set debug value for output
# 3=extreme, 2=verbose, 1=standard, 0=none
debug = 2

# set to 1 for unique LED (only one lit at any time). Yet to be implemented
unique_led = 0

# pins in use
encoder_a = 23
encoder_b = 24
pushbutton = 27   # set to zero if not connected

newbutton1 = 4
newbutton2 = 5
newbutton3 = 6

newled1 = 14
newled2 = 15
newled3 = 16

# control pin - defined in hardware so do not change
mutepin = 22

# control name in amixer. IQaudIO Master volume control
control='Digital'

# volume change per encoder detent step in percent
# this can be non-integer, but it is then not completely regular
volumestepsize = 3      # percent of full span

# volume mapping - not implemented
# set to '-M' for alsamixer-like mapping
# set to '-R' for raw percentage volume
# volumemapping = ''    # must be '-M' or '-R'

# guard times for poweroff function (seconds)
holdtimemute = 4        # amp mutes after switch held this long
holdtimeoff = 6         # system powers off after this long (total)

# debounce time (on switch, not encoder)
switchdebounce = 30.    # milliseconds

# loop timing
# volume will only increment this many times per second
# regardless of how many edge transitions occur within interval
volumeloophz = 3
# button press timing loop
buttonloopwait = 0.02  # seconds, recommend 0.02


###########################################################

if debug : print "\nIQaudIO Cosmic Controller script initialising"

# initialise hardware
#   all get onboard pullup (approx 50k)
GPIO.setmode(GPIO.BCM)
GPIO.setup(encoder_a, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(encoder_b, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(pushbutton, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(newbutton1, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(newbutton2, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(newbutton3, GPIO.IN, pull_up_down=GPIO.PUD_UP)

GPIO.setup(mutepin, GPIO.OUT)
GPIO.setup(newled1, GPIO.OUT)
GPIO.setup(newled2, GPIO.OUT)
GPIO.setup(newled3, GPIO.OUT)

# Turn off all three LEDs
GPIO.output(newled1,0)
GPIO.output(newled2,0)
GPIO.output(newled3,0)

# create events callback functions

encoderpos = 0
lastencoderpos = 0
encodercount = 0
lastencodercount = 0

def encoder_callback(channel):
    global lastencoderpos
    global encodercount
    if (debug>2) : print('Edge detected on encoder channel %s'%channel)

    encoderpos=((GPIO.input(encoder_a)&1)<<1)|(GPIO.input(encoder_b)&1)
    c = (lastencoderpos<<2)|encoderpos
    if (c==0b1101 or c==0b0100 or c==0b0010 or c==0b1011):
        encodercount = encodercount + 1
        if (debug>2) : print("  encoder count now: %d"%encodercount)
    else:
        if (c==0b1110 or c==0b0111 or c==0b0001 or c==0b1000):
            encodercount = encodercount -1
            if (debug>2) : print("  encoder count now: %d"%encodercount)
        else:
            # there shouldn't be any else
            if (debug>2) : print("  unrecognised encoder state discarded")
            pass
        
    lastencoderpos=encoderpos

    
def pushbutton_callback(channel):
    if (debug>1) : print('Edge detected on button channel %s'%channel)
    # record time press was detected
    presstime = datetime.datetime.utcnow()
    # wait until released by polling
    while (GPIO.input(channel)==0):
        pressduration=(datetime.datetime.utcnow()-presstime).total_seconds()
        if (pressduration>float(holdtimemute)):
            if debug : print('Muting amplifier due to button hold')
            GPIO.output(mutepin,0)
        sleep (buttonloopwait)
    # find final duration
    pressduration=(datetime.datetime.utcnow()-presstime).total_seconds()

    if (pressduration>(float(holdtimeoff))):
        if debug : print ('Sytem shutdown due to button hold')
# Uncomment line below for shutdown to be supported.            
#        subprocess.call(['shutdown -h now "System halted by volume control" &'], shell=True)
    else:    
        if ( (pressduration>(float(switchdebounce)/1000.0)) and (pressduration<float(holdtimemute)) ):
            if debug : print('Toggle mute state')
            if (debug>1) : print("  button pressed for %f seconds" %pressduration)
            
            if (GPIO.input(mutepin)==0):
                GPIO.output(mutepin,1)
            else:
                GPIO.output(mutepin,0)

            if (debug>1):
               call(["/usr/bin/amixer", "sset", control, "toggle"])

            else:
               call(["/usr/bin/amixer", "-q", "sset", control, "toggle"])



def newbutton1_callback(channel):
    if (debug>1) : print('Edge detected on button1 channel %s'%channel)
    # record time press was detected
    presstime = datetime.datetime.utcnow()
    # wait until released by polling
    while (GPIO.input(channel)==0):
        pressduration=(datetime.datetime.utcnow()-presstime).total_seconds()
        sleep (buttonloopwait)

    # find final duration
    pressduration=(datetime.datetime.utcnow()-presstime).total_seconds()

    if ( (pressduration>(float(switchdebounce)/1000.0)) and (pressduration<float(holdtimemute)) ):
        if debug : print('Toggle led 1 state')
        if (debug>1) : print("  button pressed for %f seconds" %pressduration)
            
        if (GPIO.input(newled1)==0):
            GPIO.output(newled1,1)
        else:
            GPIO.output(newled1,0)




def newbutton2_callback(channel):
    if (debug>1) : print('Edge detected on button1 channel %s'%channel)
    # record time press was detected
    presstime = datetime.datetime.utcnow()
    # wait until released by polling
    while (GPIO.input(channel)==0):
        pressduration=(datetime.datetime.utcnow()-presstime).total_seconds()
        sleep (buttonloopwait)

    # find final duration
    pressduration=(datetime.datetime.utcnow()-presstime).total_seconds()

    if ( (pressduration>(float(switchdebounce)/1000.0)) and (pressduration<float(holdtimemute)) ):
        if debug : print('Toggle led 2 state')
        if (debug>1) : print("  button pressed for %f seconds" %pressduration)
        
        if (GPIO.input(newled2)==0):
            GPIO.output(newled2,1)
        else:
            GPIO.output(newled2,0)



def newbutton3_callback(channel):
    if (debug>1) : print('Edge detected on button1 channel %s'%channel)
    # record time press was detected
    presstime = datetime.datetime.utcnow()
    # wait until released by polling
    while (GPIO.input(channel)==0):
        pressduration=(datetime.datetime.utcnow()-presstime).total_seconds()
        sleep (buttonloopwait)

    # find final duration
    pressduration=(datetime.datetime.utcnow()-presstime).total_seconds()

    if ( (pressduration>(float(switchdebounce)/1000.0)) and (pressduration<float(holdtimemute)) ):
        if debug : print('Toggle led 3 state')
        if (debug>1) : print("  button pressed for %f seconds" %pressduration)
            
        if (GPIO.input(newled3)==0):
            GPIO.output(newled3,1)
        else:
            GPIO.output(newled3,0)




    
# register callback functions
GPIO.add_event_detect(encoder_a, GPIO.BOTH, callback=encoder_callback)
GPIO.add_event_detect(encoder_b, GPIO.BOTH, callback=encoder_callback)

GPIO.add_event_detect(pushbutton, GPIO.FALLING, callback=pushbutton_callback, bouncetime=int(switchdebounce))
GPIO.add_event_detect(newbutton1, GPIO.FALLING, callback=newbutton1_callback, bouncetime=int(switchdebounce))
GPIO.add_event_detect(newbutton2, GPIO.FALLING, callback=newbutton2_callback, bouncetime=int(switchdebounce))
GPIO.add_event_detect(newbutton3, GPIO.FALLING, callback=newbutton3_callback, bouncetime=int(switchdebounce))


# main code loop - we look out for changes in encoder position and
# update volume control accordingly
if debug : print "IQaudIO Cosmic Controller script running"

mln=0
try:
    while True:
        # this is an empty infinite loop but count it anyway
        mln = (mln +1)%1000
        if debug>2 : print "%03d main loop" %mln

        if (encodercount <> lastencodercount):
            if (debug>1) : print "encoder count change to %d"%encodercount

            # decide on step size
            if (encodercount > lastencodercount):
                v = str(volumestepsize)+'%+'
            else:
                v = str(volumestepsize)+'%-'

            # implement step size, eitehr with or without report to user
            # Uncomment below to adjust volume control.
            if (debug>1):
               call(["/usr/bin/amixer", "sset", control, v])

            else:
               call(["/usr/bin/amixer", "-q", "sset", control, v])
                    
            lastencodercount=encodercount

        sleep (1.0/volumeloophz)

except KeyboardInterrupt:
    pass
    
# clean up after keyboard interrup
if debug : print "\nIQaudIO Cosmic Controller script exiting"
GPIO.cleanup()

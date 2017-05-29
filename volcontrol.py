#!/usr/bin/python
#
# $Id: VolControl.py,v 1.10 2017/03/04 14:43:38 ian Exp $
#
# Adjust ALSA volume by means of rotary encoder
# also incorporates press-switch
#   click = toggle mute
#   hold = shutdown machine
#
# hardware:
#   rotary encoder with common tied to earth
#   momentary switch with one termianl tied to earth
#   all input will use raspberry internal pullups


from time import sleep
from subprocess import call
import datetime
import subprocess
import RPi.GPIO as GPIO

# set debug value for output
# 3=extreme, 2=verbose, 1=standard, 0=none
debug = 1

# pins in use
encoder_a = 23
encoder_b = 24
pushbutton = 27   # set to zero if not connected

# control pin - defined in hardware so do not change
mutepin = 22

# control name in amixer
control='Digital'

# volume change per encoder detent step in percent
# this can be non-integer, but it is then not completely regular
volumestepsize = 3      # percent of full span

# volume mapping
# set to '-M' for alsamixer-like mapping
# set to '-R' for raw percentage volume
volumemapping = '-M'    # must be '-M' or '-R'

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

if debug : print "\nVolControl initialising"

# initialise hardware
#   all get onboard pullup (approx 50k)
GPIO.setmode(GPIO.BCM)
GPIO.setup(encoder_a, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(encoder_b, GPIO.IN, pull_up_down=GPIO.PUD_UP)
if pushbutton:
    GPIO.setup(pushbutton, GPIO.IN, pull_up_down=GPIO.PUD_UP)

GPIO.setup(mutepin, GPIO.OUT)

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
        subprocess.call(['shutdown -h now "System halted by volume control" &'], shell=True)
    else:    
        if ( (pressduration>(float(switchdebounce)/1000.0)) and (pressduration<float(holdtimemute)) ):
            if debug : print('Toggle mute state')
            if (debug>1) : print("  button pressed for %f seconds" %pressduration)
            
            if (GPIO.input(mutepin)==0):
                GPIO.output(mutepin,1)
            else:
                GPIO.output(mutepin,0)
    
# register callback functions
GPIO.add_event_detect(encoder_a, GPIO.BOTH, callback=encoder_callback)
GPIO.add_event_detect(encoder_b, GPIO.BOTH, callback=encoder_callback)
if pushbutton :
    GPIO.add_event_detect(pushbutton, GPIO.FALLING, callback=pushbutton_callback, bouncetime=int(switchdebounce))


# main code loop - we look out for changes in encoder position and
# update volume control accordingly
if debug : print "VolControl running"

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
            if (debug>1):
                call(["/usr/bin/amixer", volumemapping, "set", control, v])
            else:
                call(["/usr/bin/amixer", "-q", volumemapping, "set", control, v])
                    
            lastencodercount=encodercount

        sleep (1.0/volumeloophz)

except KeyboardInterrupt:
    pass
    
# clean up after keyboard interrup
if debug : print "\nVolControl exiting"
GPIO.cleanup()

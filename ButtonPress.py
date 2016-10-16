#!/usr/bin/env python2.7

# Key press detect code, uses gpio 27 and ground to a momentary switch. If pressed for more 
# than 1sec but (less than 5) it will force a reboot.
# If pressed for more than 5 seconds it will force a shutdown
# Code taken from sample posted on the Foundation's forum
# by paulv >> Fri Jun 28, 2013 2:58 pm
# https://www.raspberrypi.org/forums/viewtopic.php?t=48455&p=379280
 

from time import sleep
import subprocess
import RPi.GPIO as GPIO

# GPIO channel 27 is on pin 13 of 40way connector
# with GND on pin14
CHANNEL = 27

GPIO.setmode(GPIO.BCM)
GPIO.setup(CHANNEL, GPIO.IN, pull_up_down=GPIO.PUD_UP)
# setup the channel as input with a 50K Ohm pull up. A push button will ground the pin,
# creating a falling edge.


def system_action(CHANNEL):
    print('Button press = negative edge detected on channel %s'%CHANNEL)
    button_press_timer = 0
    while True:
            if (GPIO.input(CHANNEL) == False) : # while button is still pressed down
                button_press_timer += 1 # keep counting until button is released
            else: # button is released, figure out for how long
                if (button_press_timer > 5) : # pressed for > 5 seconds
                    print "long press > 5 : ", button_press_timer
                    # do what you need to do before halting
                    subprocess.call(['shutdown -h now "System halted by GPIO action" &'], shell=True)
                elif (button_press_timer > 1) : # press for > 1 < 5 seconds
                    print "short press > 1 < 5 : ", button_press_timer
                    # do what you need to do before a reboot
                    subprocess.call(['sudo reboot &'], shell=True)
                button_press_timer = 0
            sleep(1)

GPIO.add_event_detect(CHANNEL, GPIO.FALLING, callback=system_action, bouncetime=200)
# setup the thread, detect a falling edge on gpio and debounce it with 200mSec

# assume this is the main code...
try:
    while True:
        # do whatever
        # while "waiting" for falling edge on gpio
        sleep (2)

except KeyboardInterrupt:
    GPIO.cleanup()       # clean up GPIO on CTRL+C exit
GPIO.cleanup()           # clean up GPIO on normal exit

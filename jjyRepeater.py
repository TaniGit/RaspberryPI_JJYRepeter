#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import signal
import RPi.GPIO as GPIO
import datetime
import time
import sys

global pwm

def send_bit(bit):
    now = datetime.datetime.now()
    usec = now.microsecond

    time.sleep(1-(usec/1000000.0))

    if bit == -1:
        pwm.start(50)
        time.sleep(0.199)
        pwm.start(0)
    elif bit == 0:
        pwm.start(50)
        time.sleep(0.799)
        pwm.start(0)
    elif bit == 1:
        pwm.start(50)
        time.sleep(0.499)
        pwm.start(0)

def mkjjyCode():
    now = datetime.datetime.now()
    minute = now.minute
    hour   = now.hour
    day    = now.toordinal() - datetime.date(now.year, 1, 1).toordinal() 
    year   = now.year % 100
    wday   = now.isoweekday() % 7
    sec    = now.second

    jjyCode = [-1]*60

    jjyCode[ 0] = -1
    jjyCode[ 1] = (((minute    /  10) & 0x4) >> 2)
    jjyCode[ 2] = (((minute    /  10) & 0x2) >> 1)
    jjyCode[ 3] =  ((minute    /  10) & 0x1)
    jjyCode[ 4] =  0
    jjyCode[ 5] = (((minute    %  10) & 0x8) >> 3)
    jjyCode[ 6] = (((minute    %  10) & 0x4) >> 2)
    jjyCode[ 7] = (((minute    %  10) & 0x2) >> 1)
    jjyCode[ 8] =  ((minute    %  10) & 0x1)
    jjyCode[ 9] = -1
    jjyCode[10] =  0
    jjyCode[11] =  0
    jjyCode[12] = (((hour      /  10) & 0x2) >> 1)
    jjyCode[13] =  ((hour      /  10) & 0x1)
    jjyCode[14] =  0
    jjyCode[15] = (((hour      %  10) & 0x8) >> 3)
    jjyCode[16] = (((hour      %  10) & 0x4) >> 2)
    jjyCode[17] = (((hour      %  10) & 0x2) >> 1)
    jjyCode[18] =  ((hour      %  10) & 0x1)
    jjyCode[19] = -1
    jjyCode[20] =  0
    jjyCode[21] =  0
    jjyCode[22] = (((day       / 100) & 0x2) >> 1)
    jjyCode[23] =  ((day       / 100) & 0x1)
    jjyCode[24] =  0
    jjyCode[25] = ((((day /10) %  10) & 0x8) >> 3)
    jjyCode[26] = ((((day /10) %  10) & 0x4) >> 2)
    jjyCode[27] = ((((day /10) %  10) & 0x2) >> 1)
    jjyCode[28] =  (((day /10) %  10) & 0x1)
    jjyCode[29] = -1
    jjyCode[30] = (((day       %  10) & 0x8) >> 3)
    jjyCode[31] = (((day       %  10) & 0x4) >> 2)
    jjyCode[32] = (((day       %  10) & 0x2) >> 1)
    jjyCode[33] =  ((day       %  10) & 0x1)
    jjyCode[34] =  0
    jjyCode[35] =  0
    jjyCode[36] = jjyCode[12] ^ jjyCode[13] ^ jjyCode[15] ^ jjyCode[16] ^ jjyCode[17] ^ jjyCode[18]
    jjyCode[37] = jjyCode[ 1] ^ jjyCode[ 2] ^ jjyCode[ 3] ^ jjyCode[ 5] ^ jjyCode[ 6] ^ jjyCode[ 7] ^ jjyCode[ 8]
    jjyCode[38] =  0
    jjyCode[39] = -1
    jjyCode[40] =  0
    jjyCode[41] = (((year      /  10) & 0x8) >> 3)
    jjyCode[42] = (((year      /  10) & 0x4) >> 2)
    jjyCode[43] = (((year      /  10) & 0x2) >> 1)
    jjyCode[44] =  ((year      /  10) & 0x1)
    jjyCode[45] = (((year      %  10) & 0x8) >> 3)
    jjyCode[46] = (((year      %  10) & 0x4) >> 2)
    jjyCode[47] = (((year      %  10) & 0x2) >> 1)
    jjyCode[48] =  ((year      %  10) & 0x1)
    jjyCode[49] = -1
    jjyCode[50] =  ((wday             & 0x4) >> 2)
    jjyCode[51] =  ((wday             & 0x2) >> 1)
    jjyCode[52] =   (wday             & 0x1)
    jjyCode[53] =  0
    jjyCode[54] =  0
    jjyCode[55] =  0
    jjyCode[56] =  0
    jjyCode[57] =  0
    jjyCode[58] =  0
    jjyCode[59] = -1

    return jjyCode

def exit_handler(signal, frame):
    pwm.stop()
    GPIO.cleanup()
    sys.exit(0)

##################
###### MAIN ######
##################

signal.signal(signal.SIGINT, exit_handler)

PWM_PIN = 18
JJY_Hz  = 40 * 1000

GPIO.setmode(GPIO.BCM)
GPIO.setup(PWM_PIN,GPIO.OUT)

pwm = GPIO.PWM(PWM_PIN,JJY_Hz)
pwm_run = 0

while True:
    now = datetime.datetime.now()
    minute = now.minute
    sec = now.second
    usec = now.microsecond

    wait_time = 59.1 - (sec + usec/1000000.0)
    if wait_time > 0:
        time.sleep(wait_time)

    jjyCode = mkjjyCode()
    for i in range(60):
        send_bit(jjyCode[i])

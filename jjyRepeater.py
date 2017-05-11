#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import signal
import RPi.GPIO as GPIO
import datetime
import time
import sys

global pwm,pwm_run

def send_bit(bit):
    if bit == -1: # マーカ
        pwm_run = 1
        pwm.start(50)
        time.sleep(0.199)
        pwm.stop()
        pwm_run = 0
        time.sleep(0.799)
    elif bit == 0: # 0
        pwm_run = 1
        pwm.start(50)
        time.sleep(0.799)
        pwm.stop()
        pwm_run = 0
        time.sleep(0.199)
    elif bit == 1: # 1
        pwm_run = 1
        pwm.start(50)
        time.sleep(0.499)
        pwm.stop()
        pwm_run = 0
        time.sleep(0.499)

def send_bcd(num, count, parity=0):
    for i in range(count):
        bit = (num >> ((count-1) - i)) & 0x1
        send_bit(bit)
        parity ^= bit
    return parity

def send_datetime(now):
    now = datetime.datetime.now()
    minute = now.minute
    hour = now.hour
    day = now.toordinal() - datetime.date(now.year, 1, 1).toordinal() 
    year = now.year % 100
    wday = now.isoweekday() % 7
    sec = now.second
    usec = now.microsecond
 
    min_parity = 0
    hour_parity = 0
 
    ############################################################
    send_bit(-1)
    
    # 10分位のBCD
    min_parity = send_bcd(int(minute/10), 3, min_parity)
    
    send_bit(0)
    
    # 1分位のBCD
    min_parity = send_bcd(minute%10, 4, min_parity)
    
    send_bit(-1)
 
    ############################################################
    send_bit(0)
    send_bit(0)
    
    # 10時位のBCD
    hour_parity = send_bcd(int(hour/10), 2, hour_parity)
    
    send_bit(0)
    
    # 1時位のBCD
    hour_parity = send_bcd(hour%10, 4, hour_parity)
    
    send_bit(-1)
    
    ############################################################
    send_bit(0)
    send_bit(0)
    
    # 累計日数100日位のBCD
    send_bcd(int(day/100), 2)
    
    send_bit(0)
 
    # 累計日数10日位のBCD
    send_bcd(int((day%100) / 10), 4)
    
    send_bit(-1)
 
    ############################################################
    # 累計日数1日位のBCD    
    send_bcd(day%10, 4)
    
    send_bit(0)
    send_bit(0)
    
    # パリティ
    send_bit(hour_parity)
    send_bit(min_parity)
    
    send_bit(0)
    send_bit(-1)
 
    ############################################################
    send_bit(0)
    
    # 西暦年10年位のBCD
    send_bcd(int((year%100)/10), 4)
    
    # 西暦年1年位のBCD
    send_bcd(year%10, 4)
    
    send_bit(-1)
 
    ############################################################
    # 曜日のBCD
    send_bcd(wday, 3)
    
    send_bit(0)
    send_bit(0)
    send_bit(0)
    send_bit(0)
    send_bit(0)
    send_bit(0)
 
    # マーカ
    pwm_run = 1
    pwm.start(50)
    time.sleep(0.199)
    pwm.stop()
    pwm_run = 0
    # 0.8 秒残しておき，次回呼び出しタイミングの調整代とする
 
def exit_handler(signal, frame):
        # Ctrl+Cが押されたときにデバイスを初期状態に戻して終了する。
        if pwm_run != 0:
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

    print(60 - (sec + usec/1000000.0))
 
    # 0 秒になるまで待つ
    time.sleep(60 - (sec + usec/1000000.0))
 
    send_datetime(now + datetime.timedelta(minutes=1))

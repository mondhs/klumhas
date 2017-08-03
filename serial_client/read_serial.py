#!/usr/bin/env python
from time import strftime
from time import sleep

def readSensor():
    import serial
    ser = serial.Serial('/dev/ttyUSB0', baudrate=9600, timeout =1)
    sleep(3)
    ser.write('DATA?\n')
    sleep(2)
    msg = ser.read(ser.inWaiting())
    return "time:"+strftime("%Y%m%d_%H%M%S")+";"+msg.strip()

def transform(message):
    messageArr = message.rstrip(";").split(";")
    messageMap = dict(item.split(":") for item in message.rstrip(";").split(";"))
    return ";".join([messageMap["time"],messageMap["temp0"],messageMap["temp1"],messageMap["light"]])

msg="time:20161027_202517;temp0:32.63;temp1:29.00;humid:10.25;light:326;paramNo:4;"
msg=readSensor()
print transform(msg)


#!/usr/bin/env python3
import serial
import re
import json
from time import sleep

def readSensor():
    ser = serial.Serial('/dev/ttyUSB0', baudrate=9600, timeout =0)
    sleep(2)
    ser.write('STATE?lamp1=on;\n'.encode())
    sleep(2)
    msg = ser.readall().strip()
    #msg = re.sub(r';temp.*$', "", msg)
    return msg.strip()

def transform(message):
    messageArr = message.rstrip(";").split("\r\n")
    messageMap = dict(item.split("=") for item in messageArr)
    return messageMap
    #return ";".join([messageMap["time"],messageMap["temp0"],messageMap["temp1"],messageMap["light"]])

msg=readSensor()
print (msg)
#messageMap = transform(msg)
#print (json.dumps(messageMap))


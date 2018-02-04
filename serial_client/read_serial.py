#!/usr/bin/env python3
from time import strftime
from time import sleep

def readSensor():
    import serial
    ser = serial.Serial('/dev/ttyUSB0', baudrate=9600, timeout =0)
    sleep(3)
    ser.write('DATA?\n'.encode())
    sleep(2)
    msg = ser.readall().decode()#.replace("\n",";")
    return "time:"+strftime("%Y%m%d_%H%M%S")+"\n"+msg.strip()


msg="time:20161027_202517;temp0:32.63;temp1:29.00;humid:10.25;light:326;paramNo:4;"
msg=readSensor()
print(msg)


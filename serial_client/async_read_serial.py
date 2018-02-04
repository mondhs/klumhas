#!/usr/bin/env python3
import asyncio
import serial
import time
import re

@asyncio.coroutine
def my_coroutine(task_name, future):
    with serial.Serial('/dev/ttyUSB0', baudrate=9600, timeout=0, write_timeout=0) as ser:
        if not ser.is_open:
            print(ser.is_open)
            ser.open()
        yield from asyncio.sleep(2)
        ser.write(b'DATA?\n')
        ser.flush()
        yield from asyncio.sleep(2)
        line=[]
        while (ser.in_waiting) or (len(line)==0):
            for c in ser.read(1):
                line.append(chr(c))
        msg = re.sub(r';temp.*$', "", "".join(line)).strip()
        #print(msg)
        messageMap = dict(item.split("=") for item in msg.split('\r\n'))
        future.set_result(messageMap)
        

def got_result(future):
    print(future.result())

loop = asyncio.get_event_loop()
future1 = asyncio.Future()
tasks = [
    my_coroutine('task1', future1)]
future1.add_done_callback(got_result)
loop.run_until_complete(asyncio.wait(tasks))
loop.close()

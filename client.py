import zmq
import sys
import random
context = zmq.Context()
print("Connecting to server...")
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")
import msg_pb2
import datetime
import time
import os
_msg = msg_pb2.Msg()

_msg.sequence = 1

seq = _msg.sequence

while True:
    
    _msg.sequence=_msg.sequence + 1
    _msg.payload = str(random.randrange(0,101))
    seq=_msg.sequence
    serial = _msg.SerializeToString()

    #print(serial,type(serial))

# _msg.ParseFromString(serial)
    t_dict = {}
    start = time.monotonic() * 1000
    t_dict[seq] = start
    #input1 = input("in: ").strip()
    if seq == 100:
        sys.exit()
    socket.send(serial)

    message = socket.recv()
    _msg.ParseFromString(message)
    
    end = time.monotonic() * 1000

    seq_back = _msg.sequence
    
    RTT =  (end - t_dict[seq_back])
    print("Received reply: "+_msg.payload+" seq: "+str(seq_back)+" RTT: "+"{:.3f}".format(RTT)+" f: "+ str(t_dict[seq_back])+ " b: "+ str(end))
    time.sleep(0.01)

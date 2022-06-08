import zmq
import sys
context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:5555")
import msg_pb2
while True:
    _msg = msg_pb2.Msg()
    try:
        print("wait for client ...")
        message = socket.recv()
        _msg.ParseFromString(message)
        print("message from client:", _msg.payload)
        socket.send(message)
    except Exception as e:
        print('exception:',e)
        sys.exit()

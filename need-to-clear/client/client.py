import zmq  
  
context = zmq.Context()  
socket = context.socket(zmq.REP)  
socket.bind("tcp://*:5555")  

message = socket.recv()  
print("message from client:", message)  

#  Send reply back to client  
socket.send("World")    

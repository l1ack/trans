import zmq
import time
import math

def accept_data_from_driver(param_dict: dict):
    for case_id in ub_dict:
        for user_id in ub_dict[case_id]:
            user_behavior = ub_dict[case_id][user_id]
            interval_seq = user_behavior['interval_seq']
            size_seq = user_behavior['size_seq']
            user_ip = user_behavior['user_ip']
            for idx, interval in enumerate(interval_seq):
                interval_ms = interval * (10 ** -3)
                current_size = size_seq[idx]
                time.sleep(interval_ms)
                send_data_2_go_server(current_size, user_ip)
            
def send_data_2_go_server(size: int, user_ip: str):
    context = zmq.Context()  
    print("Connecting to server...")  
    socket = context.socket(zmq.REQ)  
    socket.connect ("tcp://localhost:5555")  
    socket.send_string(user_ip)  

# message = socket.recv()  
# print("Received reply: ", message)

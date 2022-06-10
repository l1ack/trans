#!/usr/bin/env python
# coding: utf-8

# In[6]:


# Python program to read
# json file
import json
import random
import math


def generate_interval_n_size_sequence_by_json(filename: str) -> dict:
    """Generate interval and size sequence for the users inside the cases
    
    Generate interval and size sequence for the users inside the cases, this function directly accept the json file, and do parsing
    to fetch key-value pairs in json, and generate python dictionary for loading these data.
    This function is going to read the dictionary while doing the interval and size sequence generation for each user in each case. 
    
    Args:
        filename: the file name of the json file, which must be obey to the sample json format
    
    Returns:
        A system generated dictionary which include the system generated interval sequence and size sequence for each case
        
        {Case_id: {
            User_id: {
                'interval_seq': [interval1, interval2, interval3, ...],
                'size_seq': [size1, size2, size3, ... ],
                'user_ip': '127.0.0.1'
                }}}
        
        Returned dictionary key can be integer when it is case_id and user_id, or string when it is the key for fetching 
        interval sequence and time sequence and the value of the key can be dictionary or list depends on the layer to be obtained
    """
    
    # Opening JSON file
    f = open(filename)

    # returns JSON object as a dictionary
    data = json.load(f)
    interval_n_size_sequence = {} # user_behavior_dict['userid'] = behavior dict
    
    for case in data['cases']:
        case_id = case['case_id']
        timeline = case['timeline']
        tmp_intv_n_size_seq_dict = {}
        
        for user in case['users']:
            user_id = user['userid']
            user_ip = user['userip']
            behavior = {
                'interval_seq_pattern': user['interval_seq_pattern'],
                'min_interval': user['min_interval'],
                'max_interval': user['max_interval'],
                'custom_interval_seq': user['custom_interval_seq'],
                'protocol': user['protocol'],
                'size_seq_pattern': user['size_seq_pattern'],
                'min_size': user['min_size'],
                'max_size': user['max_size'],
                'custom_size_seq': user['custom_size_seq']
            }
            tmp_intv_n_size_seq_dict[user_id] = generate_interval_n_size_sequence(timeline,behavior)
            tmp_intv_n_size_seq_dict[user_id]['user_ip'] = user_ip
            
        interval_n_size_sequence[case_id] = tmp_intv_n_size_seq_dict

    # Closing file
    f.close()
    return interval_n_size_sequence


def generate_interval_n_size_sequence(timeline: int,behavior: dict) -> dict:
    """Generate interval and size sequence
    
    Generate interval sequence and size sequence depends on the user behavior dictionary
    
    Args:
        timeline: the total time to be consumed for the test case
        behavior: the user behavior defined in the json file
    
    Returns:
        A system generated dictionary which include the system generated interval sequence and size sequence
        
        {'interval_seq': [interval1, interval2, interval3, ...],
            'size_seq': [size1, size2, size3, ... ]}
        
        Returned key is always string, and the value of the key is always list.
        The returned interval sequence list and size sequence list is the same size
    """
    
    tmp_intv_n_size_dict = {}
    
    # interval related params
    interval_seq_pattern = behavior['interval_seq_pattern']
    min_i = behavior['min_interval']
    max_i = behavior['max_interval']
    custom_interval_seq = behavior['custom_interval_seq']

    # size related params
    size_seq_pattern = behavior['size_seq_pattern']
    min_s = behavior['min_size']
    max_s = behavior['max_size']
    custom_size_seq = behavior['custom_size_seq']

    # protocol related params
    protocol = behavior['protocol']
    
    # generate interval sequence pattern
    custom_interval_sum = sum(custom_interval_seq)
    tmp_intv_n_size_dict["interval_seq"] = generate_interval_sequence(interval_seq_pattern, timeline, custom_interval_sum, 
                                                                          custom_interval_seq , min_i, max_i)

    # generate size sequence pattern
    final_interval_seq_len = len(tmp_intv_n_size_dict["interval_seq"])
    tmp_intv_n_size_dict["size_seq"] = generate_size_sequence(size_seq_pattern, timeline, custom_interval_sum,custom_size_seq, 
                                                              final_interval_seq_len, min_s, max_s)
        
    return tmp_intv_n_size_dict        


def generate_interval_sequence(pattern: str, timeline: int, interval_sum: int, custom_interval_list: list,
                               min_i: int, max_i: int) -> list:
    """Generate a interval sequence
    
    Generate a interval sequence depends on different pattern type
    
    Args:
        pattern: a string that indicate the pattern for size sequence generation
        timeline: the total time to be consumed for the test case
        interval_sum: the sum of elements of user customized interval sequence
        custom_interval_list: the user customized interval sequence which can be fetched from json file
        min_s: the min size of the frame -> defined in json file
        max_s: the max size of the frame -> defined in json file
    
    Returns:
        A system generated size sequence list based on indicated pattern
        example:
        
        [interval1, interval2, interval3, ...]
        
        Returned element inside the list is always integer, the integer is to indicate the interval to send frame by "ms"
        
    Raises:
        ValueError: An error occurred when giving wrong pattern string
    """
    
    # generate interval sequence pattern
    interval_seq = []
    
    if pattern == 'custom':
        total_repeat = math.ceil(timeline / interval_sum) # take ceil of float as total replication
        interval_seq = custom_interval_list * total_repeat

    elif pattern == 'random':
        tmp_total_intv = 0
        while tmp_total_intv < timeline:
            tmp_intv = random.randrange(min_i, max_i + 1)
            tmp_total_intv += tmp_intv
            interval_seq.append(tmp_intv)
            
    else:
        raise ValueError('Invalid size pattern mode !!!')
    
    return interval_seq


def generate_size_sequence(pattern: str, timeline: int, interval_sum: int, custom_size_list: list, 
                           interval_seq_len: int, min_s: int, max_s: int) -> list:
    """Generate a size sequence
    
    Generate a size sequence depends on different pattern type, this function must be used after 
    the interval sequence has been generated by system
    
    Args:
        pattern: a string that indicate the pattern for size sequence generation
        timeline: the total time to be consumed for the test case
        interval_sum: the sum of elements of user customized interval sequence
        custom_size_list: the user customized size sequence which can be fetched from json file
        interval_seq_len: the length of system-generated interval sequence
        min_s: the min size of the frame -> defined in json file
        max_s: the max size of the frame -> defined in json file
    
    Returns:
        A system generated size sequence list based on indicated pattern
        example:
        
        [size1, size2, size3, ...]
        
        Returned element inside the list is always integer, the integer is to indicate the bytes of the frame
        
    Raises:
        ValueError: An error occurred when giving wrong pattern string
    """
    
    # generate size sequence pattern
    size_seq = []
#     print("interval {}".format(interval_seq_len))
    
    if pattern == 'custom':
        size_seq = custom_size_list * interval_seq_len
        size_seq = size_seq[: interval_seq_len]
        
    elif pattern == 'random':
        for cnt in range(0, interval_seq_len):
            tmp_size = random.randrange(min_s, max_s + 1)
            size_seq.append(tmp_size)

    else:
        raise ValueError('Invalid size pattern mode !!!')
    
#     print("size pattern:{} - size{} - interval {}".format(pattern, len(size_seq), interval_seq_len))
    
    return size_seq
    
    
# Entry point here
if __name__ == '__main__':
    filename = "sample_json.json" # plug your json file name here
    ub_dict = generate_interval_n_size_sequence_by_json(filename)


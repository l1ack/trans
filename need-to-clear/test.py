import msg_pb2

_msg = msg_pb2.Msg()

_msg.sequence = 1
_msg.payload = "helloworld"

serial = _msg.SerializeToString()
print(serial,type(serial))

_msg.ParseFromString(serial)

print("m_seq{},m_payoad{}".format(_msg.sequence,_msg.payload))


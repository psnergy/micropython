
# x=b'\x08\x32'
# k = uio.BytesIO(x)
# d=protobuf.decode("m2s_SOR", k)


# encode data
# msg_fields = [{"entity_id":21, "data": 20.70, "unit_id": 2}, {"entity_id":2, "data": 20.70, "unit_id": 1}]
# stream = uio.BytesIO()
# msg = "s2m_data"
# encoded_msg = protobuf.encode(msg_fields, msg, stream).getvalue()
# print(encoded_msg)

from pyb import I2C, delay
import uio, protobuf

"""
I2C device configuration and initialization. This should be part of the superloop file.
"""
SLAVE_ADDRESS = 0x2
BAUDRATE = 100000
i2c_slave = I2C(1, I2C.SLAVE, addr=SLAVE_ADDRESS, baudrate=BAUDRATE)

"""
Global configuration options (MDR) for this module. These get encoded and sent to the master during handshake.
"""
MODULE_VERSION   = 0.1
MODULE_ID        = 2
MODULE_CLASS     = 3
ENTITY_ID        = 1
SUBS_MODULE_IDS  = 0x1F
SUBS_I2C_ADDRS   = 0


def get_i2c_data(i2c_device, recv_size):
    while True:
        try:
            data = i2c_device.recv(recv_size, timeout=100)
            print("RECV: %r" % data)
            return data
        except OSError as exc:
            if exc.args[0] not in (5, 110):
                # 5 == EIO, occurs when master does a I2C bus scan
                # 110 == ETIMEDOUT
                print("Got nothing")
        except KeyboardInterrupt:
            print("Ending")
            return 0
            
def send_i2c_data(i2c_device, data):
    while True:
        try:
            i2c_device.send(data, timeout=100)
            return True
        except:
            continue
    return False

def handshake(i2c_device):
    """
    This function handles the intial handshake with master

    @param i2c_device The i2c_device initialized as slave
    @return True if handshake was successful, false otherwise
    """
    
    stream = uio.BytesIO()
    message_fields = {"MDR_version": MODULE_VERSION, "module_id": MODULE_ID, "module_class": MODULE_CLASS, "entity_id": ENTITY_ID, "subs_module_ids": SUBS_MODULE_IDS, "subs_i2c_addrs": SUBS_I2C_ADDRS}
    msg = "s2m_MDR_response"
    
    encoded_MDR = protobuf.encode(message_fields, msg, stream).getvalue()
    MDR_len = len(encoded_MDR)
    print("Length: {}, MDR: ".format(MDR_len))
    print(encoded_MDR)
    
    MDR_ack = bytearray([MDR_len, 0xFF])
        
    data = get_i2c_data(i2c_device, 2)
    status = send_i2c_data(i2c_device, MDR_ack)
    data = get_i2c_data(i2c_device, 2)
    status = send_i2c_data(i2c_device, encoded_MDR)
    
    return status

    
def send_datapoint(data, i2c_device):
    """
    This function handles encoding data, receiving the SOR code 
    and sending data to master.
    Call this function every time a datapoint is generated.

    @param data A list of dictionaries which contain fields 
    that are expected by the proto format.
    """
    stream = uio.BytesIO()
    msg = "s2m_data"
    encoded_data = protobuf.encode(data, msg, stream).getvalue()    
    enc_data_len = len(encoded_data)

    SOR_buf = get_i2c_data(i2c_device, 4)
    SOR_stream = uio.BytesIO(SOR_buf)
    SOR_message=protobuf.decode("m2s_SOR", SOR_stream)

    print(SOR_message)
    
    if SOR_message['SOR_code'] == 1:
        doc_fields = {'doc_code':5, 'tx_length':enc_data_len}
        doc_stream = uio.BytesIO()
        doc_msg=protobuf.encode(doc_fields, 's2m_DOC', doc_stream).getvalue()
        status = send_i2c_data(i2c_device, doc_msg)
        
        data = get_i2c_data(i2c_device, 2)
        status = send_i2c_data(i2c_device, encoded_data)

        
"""
Driver code for handshake and dataflow. This should be part of the superloop.
"""    
while handshake(i2c_slave) == False:    
    continue
print("Handshake done")
delay(1000)
while True:    
    send_datapoint([{"entity_id":21, "data": 20.70, "unit_id": 2}, {"entity_id":2, "data": 20.70, "unit_id": 1}], i2c_slave)


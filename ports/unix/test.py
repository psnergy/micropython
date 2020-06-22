
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
<<<<<<< HEAD
SLAVE_ADDRESS = 0x6 # actual bus address: 0x3 (3)
=======
SLAVE_ADDRESS = 0x4 # actual bus address: 0x8
>>>>>>> e8c4195c7b72bb88e921c5ac2c0a8a311a3364ce
BAUDRATE = 100000
i2c_slave = I2C(1, I2C.SLAVE, addr=SLAVE_ADDRESS, baudrate=BAUDRATE)

"""
Global configuration options (MDR) for this module. These get encoded and sent to the master during handshake.
"""
MODULE_VERSION   = 0.2
<<<<<<< HEAD
MODULE_ID        = 7
MODULE_CLASS     = 3
ENTITY_ID        = 1
SUBS_MODULE_IDS  = 0x0
SUBS_I2C_ADDRS   = 0x0
=======
MODULE_ID        = 2
MODULE_CLASS     = 3
ENTITY_ID        = 1
SUBS_MODULE_IDS  = 0xFFFF
SUBS_I2C_ADDRS   = 0
>>>>>>> e8c4195c7b72bb88e921c5ac2c0a8a311a3364ce


def get_i2c_data(i2c_device, recv_size):
    try:
        data = i2c_device.recv(recv_size, timeout=1000)
        return data
    except:
        return None
            
def send_i2c_data(i2c_device, data):
    try:
        i2c_device.send(data, timeout=1000)
        return True
    except:
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
    
    MDR_ack = bytearray([MDR_len, 0xFF])
        
    data = get_i2c_data(i2c_device, 2)
    if data is None:
        return False
    status = send_i2c_data(i2c_device, MDR_ack)
    data = get_i2c_data(i2c_device, 2)
    if data is None:
        return False
    status = send_i2c_data(i2c_device, encoded_MDR)
    
    return status

    
def send_datapoint(data, i2c_device):
    """
    This function handles encoding data, receiving the SOR code 
    and sending data to master.
    Call this function every time a datapoint is generated.

    @param data A list of dictionaries which contain fields 
    that are expected by the proto format.
    @param i2c_device The i2c_device initialized as slave
    """
    stream = uio.BytesIO()
    msg = "s2m_data"
    encoded_data = protobuf.encode(data, msg, stream).getvalue()
    enc_data_len = len(encoded_data)
<<<<<<< HEAD
    doc_msg = bytearray([0xFF, 5, 0xFF, enc_data_len])
=======
    doc_msg = bytearray([0x0, 5, 0x0, enc_data_len])
>>>>>>> e8c4195c7b72bb88e921c5ac2c0a8a311a3364ce
    SOR_buf = get_i2c_data(i2c_device, 2)
    
    if SOR_buf is None:
        # print("SOR fail")
        return False
    print("got sor")
    if SOR_buf[0] == 1:
        if send_i2c_data(i2c_device, doc_msg) == False:
            print("doc send fail")
            return False
        print("sent doc")
        data = get_i2c_data(i2c_device, 2)
        if data is None:
            print("cts fail")
            return False
        print("got cts")
        if send_i2c_data(i2c_device, encoded_data) == False:
            print("data send fail")
            return False
        print("sent data")
        return True
    elif SOR_buf[0] == 2:
        CTS_buf = bytearray([0x0, 0x1])
        if send_i2c_data(i2c_device, CTS_buf) == False:
            print("cts send fail")
            return False
        print("got len buf")
        len_buf = get_i2c_data(i2c_device, 4)
        MDR_len = len_buf[1]+(len_buf[0]<<8)
        data_len = len_buf[3]+(len_buf[2]<<8)
        print("MDR_len: " + str(MDR_len) + " Data len: "+str(data_len))
        if send_i2c_data(i2c_device, CTS_buf) == False:
            print("cts send fail")
            return False
        
        MDR_buf  = get_i2c_data(i2c_device, MDR_len)
        data_buf = get_i2c_data(i2c_device, data_len)
        if MDR_buf is None or data_buf is None:
            print("failed to get MDR or data")
            return False
        print("Got data and MDR")        

"""
Driver code for handshake and dataflow. This should be part of the superloop.
"""
HS_DONE = False
while not HS_DONE:
    try:
        HS_DONE = handshake(i2c_slave)
<<<<<<< HEAD
    except KeyboardInterrupt:
        break;
=======
>>>>>>> e8c4195c7b72bb88e921c5ac2c0a8a311a3364ce
    except:
        continue
print("Handshake done")
# delay(1000)
while True:
    try:
        send_datapoint([{"entity_id":21, "data": 20.70, "unit_id": 2}, {"entity_id":2, "data": 20.70, "unit_id": 1}], i2c_slave)
<<<<<<< HEAD
    except KeyboardInterrupt:
        break;
=======
>>>>>>> e8c4195c7b72bb88e921c5ac2c0a8a311a3364ce
    except:
        continue
    # delay(100);





/* TODO make a callback and struct generator for pb_ostream_t that writes to micropython's stream.
 *     Details in common.h/c from nanopb client/server files
 * TODO work on reading kv pairs from a dict mp_obj_t. Details in py/objdict.c, py/obj.h
 * 
 * NOTE building requires option USER_C_MODULES=../../extmod
 *
 */


#include <stdint.h>
#include <stdlib.h>
#include "py/obj.h"
#include "py/stream.h"
#include "pb_common.h"
#include "pb_encode.h"
#include "handshake.pb.h"
#include "pb.h"

#define WRITE_VALUE(X, structname, fieldname, value) X(structname, fieldname, value)

const char err_msg[] = "wtf are you doing Alex";

STATIC mp_map_elem_t *dict_iter_next(mp_obj_dict_t *dict, size_t *cur);
STATIC mp_obj_t protobuf_encode(mp_obj_t obj, mp_obj_t stream, mp_obj_t msg_str);
pb_ostream_t pb_ostream_from_mp_stream(mp_obj_t stream);
static bool write_callback(pb_ostream_t *stream, const uint8_t *buf, size_t count);
int get_msg_id(mp_obj_t msg);
int get_msg_field_id(int msg_id, mp_obj_t msg_field);


STATIC mp_obj_t pb_enc(mp_obj_t obj, mp_obj_t msg_str) {
    int msg_id = get_msg_id(msg_str);
    if (msg_id == 0) {
	mp_raise_msg(&mp_type_ValueError, err_msg);
    }
    mp_obj_dict_t *self = MP_OBJ_TO_PTR(obj);
    mp_map_elem_t *elem = NULL;
    size_t cur = 0;
    
    while ((elem = dict_iter_next(self, &cur)) != NULL) {
	int msg_field_id = get_msg_field_id(msg_id, elem->key);	
    }
    
    return mp_obj_new_int(msg_id);
}

static bool write_callback(pb_ostream_t *stream, const uint8_t *buf, size_t count) {
    int errcode;    
    mp_stream_rw(stream->state, (void*)buf, (mp_uint_t) count, &errcode, MP_STREAM_RW_WRITE);
    if (errcode > 0) {
	return false;
    }
    return true;
}

pb_ostream_t pb_ostream_from_mp_stream(mp_obj_t stream) {
    pb_ostream_t pb_stream = {&write_callback, (void*)stream, 100, 0};
    return pb_stream;
}

int get_msg_id(mp_obj_t msg)
{    
    const char m2sMDR_req[] = "m2s_MDR_request",
	s2mMDR_req_ACK[] = "s2m_MDR_req_ACK",
	m2sMDR_res_CTS[] = "m2s_MDR_res_CTS",
	s2mMDR_response[] = "s2m_MDR_response";
    
    int id = 0; /* Default case is no matching ID */
    
    const char *msg_buf;
    msg_buf = mp_obj_str_get_str(msg);

    if (strcmp(msg_buf, m2sMDR_req) == 0)
	id = 1;
    else if (strcmp(msg_buf, s2mMDR_req_ACK) == 0)
	id = 2;
    else if (strcmp(msg_buf, m2sMDR_res_CTS) == 0)
	id = 3;
    else if (strcmp(msg_buf, s2mMDR_response) == 0)
	id = 4;
    return id;
}

int get_msg_field_id(int msg_id, mp_obj_t msg_field)
{
    const char msg1_record_type[] = "record_type",
	msg2_MDR_res_length[] = "MDR_res_length",
	msg3_timeout[] = "timeout",
	msg4_MDR_version[] = "MDR_version",
	msg4_module_id[] = "module_id",
	msg4_module_class[] = "module_class",
	msg4_entity_id[] = "entity_id",
	msg4_subscriptions[] = "subscriptions",
	msg_sub_i2c_address[] = "i2c_address";

    int id = 0;

    const char *msg_buf;
    msg_buf = mp_obj_str_get_str(msg_field);
    switch (msg_id) {
    case 2:
    {
	if (strcmp(msg2_MDR_res_length, msg_buf) == 0)
	    id = 1;
	break;
    }
    case 4:
	if (strcmp(msg4_MDR_version, msg_buf) == 0)
	    id = 1;
	else if (strcmp(msg4_module_id, msg_buf) == 0)
	    id = 2;
	else if (strcmp(msg4_module_class, msg_buf) == 0)
	    id = 3;
	else if (strcmp(msg4_entity_id, msg_buf) == 0)
	    id = 4;
	else if (strcmp(msg4_subscriptions, msg_buf) == 0)
	    id = 5;
	break;
    }
    
    return id;
}

STATIC mp_map_elem_t *dict_iter_next(mp_obj_dict_t *dict, size_t *cur) {
    size_t max = dict->map.alloc;
    mp_map_t *map = &dict->map;

    for (size_t i = *cur; i < max; i++) {
        if (mp_map_slot_is_filled(map, i)) {
            *cur = i + 1;
            return &(map->table[i]);
        }
    }

    return NULL;
}

STATIC mp_obj_t protobuf_encode(mp_obj_t obj, mp_obj_t stream, mp_obj_t msg_str)
{
    /* HOW TO ACTUALLY READ A DICTIONARY, WORK IN PROGRESS */    
    mp_obj_dict_t *self = MP_OBJ_TO_PTR(obj);
    mp_map_elem_t *elem = NULL;
    size_t cur = 0;
    elem = dict_iter_next(self, &cur);

    char test[] = "MDR_res_length";    
    const char *msg_buf;
    msg_buf = mp_obj_str_get_str(elem->key);
    printf("elem key:%s\n", msg_buf);
    
    if (strcmp(test, msg_buf) == 0) {
	return mp_obj_new_int(1);
    }
    else {
	return mp_obj_new_int(0);
    }
    return elem->key;
    
    /* DICT READ END ========================================*/

    
    mp_get_stream_raise(stream, MP_STREAM_OP_WRITE);
    /* uint32_t* arr[1] = {&response.module_id}; */
    /* *arr[0] = 2; */
    /* pb_ostream_t output = pb_ostream_from_mp_stream(stream); */
    
    /* mp_map_t map = *mp_obj_dict_get_map(obj); */
    /* if(mp_map_slot_is_filled(&map, 1)) { */
/* #define write(structname, fieldname, value) structname.fieldname=value */
/* 	WRITE_VALUE(write, response, map.table[1].key, 5); */
/* #undef write	 */
    	/* WRITE_VALUE(response, map.table[1].key, 5); */
    /* 	return mp_obj_new_int(map.table[1].key); */
    /* } */
    /* else { */
    /* 	return mp_obj_new_int(0); */
    /* } */

    /* pb_encode(&output, s2m_MDR_response_fields, &response); */
    
    /* return stream; */

    
    /* WORKING STREAM WRITE TEST */
    /* uint8_t buf[]={0x41, 0x42, 0x43}; */
    /* const char *buf; */
    /* size_t t = 4; */
    /* buf = mp_obj_str_get_str(msg_str); */
    /* const char k[] = "TEST"; */
    /* buf = mp_obj_str_get_str(msg_str); // just use this func */
    /* if (strcmp(buf, k) == 0){ // this works when string is null terminated */
    /* 	printf("YES\n"); */
    /* } */
    /* int errcode; */
    /* mp_uint_t size = 8; */
    /* printf("%s\n", buf); */
    /* mp_stream_rw(stream, (void*)buf, size, &errcode, MP_STREAM_RW_WRITE); */
    /* return stream; */
    /* } */
    /* OLDER SHIT */
    /* return mp_obj_new_int(errcode); */
    /* uint8_t buffer[20]; */
    /* s2m_MDR_response msg = s2m_MDR_response_init_default;     */
    /* pb_ostream_t pb_stream = pb_ostream_from_buffer(buffer, sizeof(buffer)); */
    
}

STATIC MP_DEFINE_CONST_FUN_OBJ_3(protobuf_encode_obj, protobuf_encode);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pb_enc_obj, pb_enc);

STATIC const mp_rom_map_elem_t protobuf_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_protobuf) },
    { MP_ROM_QSTR(MP_QSTR_encode), MP_ROM_PTR(&protobuf_encode_obj) },
    { MP_ROM_QSTR(MP_QSTR_pb_enc), MP_ROM_PTR(&pb_enc_obj) },
};


STATIC MP_DEFINE_CONST_DICT(protobuf_module_globals, protobuf_globals_table);
const mp_obj_module_t protobuf_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&protobuf_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_protobuf, protobuf_user_cmodule, MODULE_EXAMPLE_ENABLED);

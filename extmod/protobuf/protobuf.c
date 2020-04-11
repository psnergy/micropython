
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
#include "py/runtime.h"
#include "pb_common.h"
#include "pb_encode.h"
#include "handshake.pb.h"
#include "pb.h"

#define WRITE_VALUE(X, structname, fieldname, value) X(structname, fieldname, value)

typedef enum {
    M2S_MDR_REQUEST = 1,
    S2M_MDR_REQ_ACK = 2,
    M2S_MDR_RES_CTS = 3,
    S2M_MDR_RESPONSE = 4
} msg_id_t;

const char errmsg_invalid_msg[] = "Message name not found";
const char errmsg_invalid_field[] = "Invalid field for given message";
const char errmsg_encode_error[] = "Protobuf encoding error";

_subscriptions subs[32] = {_subscriptions_init_zero};
int subs_idx = 0, subs_idx_max = 0;

STATIC mp_map_elem_t *dict_iter_next(mp_obj_dict_t *dict, size_t *cur);
STATIC mp_obj_t protobuf_encode(mp_obj_t obj, mp_obj_t stream, mp_obj_t msg_str);
pb_ostream_t pb_ostream_from_mp_stream(mp_obj_t stream);
static bool write_callback(pb_ostream_t *stream, const uint8_t *buf, size_t count);
bool encode_subscription_callback(pb_ostream_t *ostream, const pb_field_t *field, void * const *arg);
int get_msg_id(mp_obj_t msg);
int get_msg_field_id(int msg_id, mp_obj_t msg_field);


STATIC mp_obj_t protobuf_encode(mp_obj_t dict, mp_obj_t msg_str, mp_obj_t stream) {
    int msg_id = get_msg_id(msg_str);
    if (msg_id == 0) {
	mp_raise_msg(&mp_type_ValueError, errmsg_invalid_msg);
    }
    
    mp_obj_dict_t *self = MP_OBJ_TO_PTR(dict);
    mp_map_elem_t *elem = NULL;
    size_t cur = 0;
    
    switch (msg_id) {
    case S2M_MDR_REQ_ACK:
	__asm__("nop");
	s2m_MDR_req_ACK ack_message = s2m_MDR_req_ACK_init_default;
	
	if ((elem = dict_iter_next(self, &cur)) != NULL) {
	    int msg_field_id = get_msg_field_id(msg_id, elem->key);
	    if (msg_field_id != 1) {
		mp_raise_msg(&mp_type_ValueError, errmsg_invalid_field);
	    }

	    /* this works on x64 unix, not sure on cortex-m arm */
	    int MDR_len = elem->value; 
	    ack_message.MDR_res_length = MDR_len;
	    pb_ostream_t output = pb_ostream_from_mp_stream(stream);

	    if (!pb_encode(&output, s2m_MDR_req_ACK_fields, &ack_message)) {
		mp_raise_msg(&mp_type_ValueError, errmsg_encode_error);
	    }

	    return stream;
	}
	else {
	    mp_raise_msg(&mp_type_ValueError, errmsg_encode_error);
	}
	break;
    case S2M_MDR_RESPONSE:
	__asm__("nop");
	s2m_MDR_response MDR_response = s2m_MDR_response_init_zero;
	
	while ((elem = dict_iter_next(self, &cur)) != NULL) {
	    int msg_field_id = get_msg_field_id(msg_id, elem->key);
	    uint32_t in = elem->value;
	    switch (msg_field_id) {
	    case 1:
		MDR_response.MDR_version = mp_obj_float_get(elem->value);
		break;
	    case 2:
		MDR_response.module_id = in;
		break;
	    case 3:
		MDR_response.module_class = in;
		break;
	    case 4:
		MDR_response.entity_id = in;
		break;
	    case 5: /* subs_module_ids */
		for (int x=1; x<32; x++) {
		    if (in&1<<x) {
			subs[subs_idx].module_id = x-1;
			subs[subs_idx++].has_module_id = true;
			printf("Got module id: %d\n", x-1);
		    }
		}
		if (subs_idx > subs_idx_max)
		    subs_idx_max = subs_idx;
		subs_idx = 0;
		break;
	    case 6: /* subs_entity_ids */
		for (int x=1; x<32; x++) {
		    if (in&(1<<x)) {
			subs[subs_idx].entity_id = x-1;
			subs[subs_idx++].has_entity_id = true;
		    }
		}
		if (subs_idx > subs_idx_max)
		    subs_idx_max = subs_idx;
		subs_idx = 0;
		break;
	    case 7:
		for (int x=1; x<32; x++) {
		    if (in&(1<<x)) {
			subs[subs_idx].i2c_address = x-1;
			subs[subs_idx++].has_i2c_address = true;
		    }
		}
		if (subs_idx > subs_idx_max)
		    subs_idx_max = subs_idx;
		subs_idx = 0;
		break;
	    }
	}
	printf("max_subs_idx: %d\n", subs_idx_max);
	pb_ostream_t output = pb_ostream_from_mp_stream(stream);
	MDR_response.subscriptions.funcs.encode=encode_subscription_callback;
	if(!pb_encode(&output, s2m_MDR_response_fields, &MDR_response)){
	    mp_raise_msg(&mp_type_ValueError, errmsg_encode_error);
	}
	return stream;
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

bool encode_subscription_callback(pb_ostream_t *ostream, const pb_field_t *field, void * const *arg)
{
    if(ostream!=NULL && field->tag == s2m_MDR_response_subscriptions_tag) {
	for (int x=0; x<subs_idx_max; x++) {
	    _subscriptions loc_subs;
	    if (subs[x].has_module_id) {
		loc_subs.has_module_id = true;
		loc_subs.module_id = subs[x].module_id;
		printf("loc_subs module id: %d\n", subs[x].module_id);
	    }
	    if (subs[x].has_i2c_address) {
		printf("here2\n");
		loc_subs.has_i2c_address = true;
		loc_subs.i2c_address = subs[x].i2c_address;
	    }
	    if (subs[x].has_entity_id) {
		loc_subs.has_entity_id = true;
		loc_subs.entity_id = subs[x].entity_id;
	    }
	    /* Module class not supported yet */
	    loc_subs.has_module_class=false;	    

	    if(!pb_encode_tag_for_field(ostream, field)){
		printf("Encode ERR1\n");
		return false;
	    }
	    if(!pb_encode_submessage(ostream, _subscriptions_fields, &loc_subs)){
		printf("Encode ERR2\n");
		return false;
	    }
	}
    }
    else{
	return false;
    }
    return true;
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
	id = M2S_MDR_REQUEST;
    else if (strcmp(msg_buf, s2mMDR_req_ACK) == 0)
	id = S2M_MDR_REQ_ACK;
    else if (strcmp(msg_buf, m2sMDR_res_CTS) == 0)
	id = M2S_MDR_RES_CTS;
    else if (strcmp(msg_buf, s2mMDR_response) == 0)
	id = S2M_MDR_RESPONSE;
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
	msg4_subs_module_ids[] = "subs_module_ids",
	msg4_subs_entity_ids[] = "subs_entity_ids",
    	msg4_subs_i2c_addrs[] = "subs_i2c_addrs";

    int id = 0;

    const char *msg_buf;
    msg_buf = mp_obj_str_get_str(msg_field);
    switch (msg_id) {
    case S2M_MDR_REQ_ACK:
    {
	if (strcmp(msg2_MDR_res_length, msg_buf) == 0)
	    id = 1;
	break;
    }
    case S2M_MDR_RESPONSE:
	if (strcmp(msg4_MDR_version, msg_buf) == 0)
	    id = 1;
	else if (strcmp(msg4_module_id, msg_buf) == 0)
	    id = 2;
	else if (strcmp(msg4_module_class, msg_buf) == 0)
	    id = 3;
	else if (strcmp(msg4_entity_id, msg_buf) == 0)
	    id = 4;
	else if (strcmp(msg4_subs_module_ids, msg_buf) == 0)
	    id = 5;
	else if (strcmp(msg4_subs_entity_ids, msg_buf) == 0)
	    id = 6;
	else if (strcmp(msg4_subs_i2c_addrs, msg_buf) == 0)
	    id = 7;
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

STATIC mp_obj_t pb_enc(mp_obj_t obj, mp_obj_t stream, mp_obj_t msg_str)
{
    /* HOW TO ACTUALLY READ A DICTIONARY */
    /* mp_obj_dict_t *self = MP_OBJ_TO_PTR(obj); */
    /* mp_map_elem_t *elem = NULL; */
    /* size_t cur = 0; */
    /* elem = dict_iter_next(self, &cur); */

    /* char test[] = "MDR_res_length";     */
    /* const char *msg_buf; */
    /* msg_buf = mp_obj_str_get_str(elem->key); */
    /* printf("elem key:%s\n", msg_buf); */
    
    /* if (strcmp(test, msg_buf) == 0) { */
    /* 	return mp_obj_new_int(1); */
    /* } */
    /* else { */
    /* 	return mp_obj_new_int(0); */
    /* } */
    /* return elem->key; */
    
    /* DICT READ END ========================================*/

    
    mp_get_stream_raise(stream, MP_STREAM_OP_WRITE);
    /* uint32_t* arr[1] = {&response.module_id}; */
    /* *arr[0] = 2; */
    pb_ostream_t output = pb_ostream_from_mp_stream(stream);
    s2m_MDR_response msg = s2m_MDR_response_init_default;
    msg.module_id = 19;
    pb_encode(&output, s2m_MDR_response_fields, &msg);
    return stream;
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
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pb_enc_obj, pb_enc);

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

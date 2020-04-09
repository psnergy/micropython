
/* TODO make a callback and struct generator for pb_ostream_t that writes to micropython's stream.
 *     Details in common.h/c from nanopb client/server files
 * TODO work on reading kv pairs from a dict mp_obj_t. Details in py/objdict.c, py/obj.h
 * 
 * NOTE building requires option USER_C_MODULES=../../extmod
 *
 */


#include <stdint.h>
#include "py/obj.h"
#include "py/stream.h"
#include "pb_common.h"
#include "pb_encode.h"
#include "handshake.pb.h"
#include "pb.h"


#define WRITE_VALUE(X, structname, fieldname, value) X(structname, fieldname, value)
s2m_MDR_response response = s2m_MDR_response_init_default;
void* fieldlist_arr[5] = {&response.module_id};


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

STATIC mp_obj_t protobuf_encode(mp_obj_t obj, mp_obj_t stream, mp_obj_t msg_str) {
    uint8_t c[]={0x41, 0x42, 0x43};
    uint8_t f[] = {0x44, 0x45, 0x46};    
    
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
    const char *buf;
    size_t t = 4;
    /* buf = mp_obj_str_get_str(msg_str); */
    const char k[] = "TEST";
    buf = mp_obj_str_get_str(msg_str); // just use this func
    if (strcmp(buf, k) == 0){ // this works when string is null terminated
	printf("YES\n");
    }
    int errcode;
    mp_uint_t size = 8;
    printf("%s\n", buf);
    mp_stream_rw(stream, (void*)buf, size, &errcode, MP_STREAM_RW_WRITE);
    return stream;
    /* } */
    /* OLDER SHIT */
    /* return mp_obj_new_int(errcode); */
    /* uint8_t buffer[20]; */
    /* s2m_MDR_response msg = s2m_MDR_response_init_default;     */
    /* pb_ostream_t pb_stream = pb_ostream_from_buffer(buffer, sizeof(buffer)); */
    
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(protobuf_encode_obj, protobuf_encode);
STATIC const mp_rom_map_elem_t protobuf_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_protobuf) },
    { MP_ROM_QSTR(MP_QSTR_encode), MP_ROM_PTR(&protobuf_encode_obj) },
};

STATIC MP_DEFINE_CONST_DICT(protobuf_module_globals, protobuf_globals_table);
const mp_obj_module_t protobuf_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&protobuf_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_protobuf, protobuf_user_cmodule, MODULE_EXAMPLE_ENABLED);

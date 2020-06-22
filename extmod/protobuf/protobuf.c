
/* DONE make a callback and struct generator for pb_ostream_t that writes to micropython's stream.
 *     Details in common.h/c from nanopb client/server files
 * DONE work on reading kv pairs from a dict mp_obj_t. Details in py/objdict.c, py/obj.h
 * 
 * NOTE building requires option USER_C_MODULES=../../extmod
 *
 */


#include <stdint.h>
#include <stdlib.h>
#include "py/obj.h"
#include "py/stream.h"
#include "py/runtime.h"
#include "py/objlist.h"
#include "py/objstr.h"
#include "py/objstringio.h"
#include "pb_common.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "handshake.pb.h"
#include "data.pb.h"
#include "protobuf.h"
#include "pb.h"

#define WRITE_VALUE(X, structname, fieldname, value) X(structname, fieldname, value)
/* #define DEBUG_PRINT */

const char errmsg_invalid_msg[] = "Message name not found";
const char errmsg_invalid_field[] = "Invalid field for given message";
const char errmsg_encode_error[] = "Protobuf encoding error";
const char errmsg_decode_error[] = "Protobuf decoding error";
const char errmsg_unsupported[] = "This feature is not supported in this release";

_subscriptions subs[32];
int subs_idx = 0, subs_idx_max = 0;
int num_datapoints = 0;               /*< This global helps the data encoding callback */
_datapoint datapoints[16];
uint8_t str_ptr = 0;
pb_byte_t str_buf[128] = {0};

STATIC mp_map_elem_t *dict_iter_next(mp_obj_dict_t *dict, size_t *cur);
STATIC mp_obj_t protobuf_encode(mp_obj_t obj, mp_obj_t stream, mp_obj_t msg_str);
pb_ostream_t pb_ostream_from_mp_stream(mp_obj_t stream);
pb_istream_t pb_istream_from_mp_stream(mp_obj_t stream);
static bool write_callback(pb_ostream_t *stream, const uint8_t *buf, size_t count);
bool encode_subscription_callback(pb_ostream_t *ostream, const pb_field_t *field, void * const *arg);
bool encode_datapoint_callback(pb_ostream_t *ostream, const pb_field_t *field, void * const *arg);
bool encode_cmd_string_callback(pb_ostream_t *ostream, const pb_field_t *field, void * const *arg);
bool decode_cmd_string_callback(pb_istream_t *istream, const pb_field_t *field, void **args);
int get_msg_id(mp_obj_t msg);
int get_msg_field_id(int msg_id, mp_obj_t msg_field);


STATIC mp_obj_t protobuf_encode(mp_obj_t dict, mp_obj_t msg_str, mp_obj_t stream) {
    int msg_id = get_msg_id(msg_str);
    if (msg_id == 0) {
	mp_raise_msg(&mp_type_ValueError, errmsg_invalid_msg);
    }
    
    switch (msg_id) {
    case S2M_MDR_REQ_ACK:
    {
	__asm__("nop");
	mp_obj_dict_t *self = MP_OBJ_TO_PTR(dict);
	mp_map_elem_t *elem = NULL;
	size_t cur = 0;
	s2m_MDR_req_ACK ack_message = s2m_MDR_req_ACK_init_default;
	
	if ((elem = dict_iter_next(self, &cur)) != NULL) {
	    int msg_field_id = get_msg_field_id(msg_id, elem->key);
	    if (msg_field_id != 1) {
		mp_raise_msg(&mp_type_ValueError, errmsg_invalid_field);
	    }

	    int MDR_len = mp_obj_get_int(elem->value); 
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
    }
    case S2M_MDR_RESPONSE:
    {
	__asm__("nop");	    
	mp_obj_dict_t *self = MP_OBJ_TO_PTR(dict);
	mp_map_elem_t *elem = NULL;
	size_t cur = 0;

	s2m_MDR_response MDR_response = s2m_MDR_response_init_default;
	
	while ((elem = dict_iter_next(self, &cur)) != NULL) {
	    int msg_field_id = get_msg_field_id(msg_id, elem->key);

	    uint32_t in = elem->value; /*< TODO change this to the proper method and test */
	    switch (msg_field_id) {
	    case 1:
		MDR_response.MDR_version = mp_obj_float_get(elem->value);
		break;
	    case 2:
		MDR_response.module_id = (in-1)/2;
		break;
	    case 3:
		MDR_response.module_class = (in-1)/2;
		break;
	    case 4:
		MDR_response.entity_id = (in-1)/2;
		break;
	    case 5: /* subs_module_ids */
		for (int x=1; x<32; x++) {
		    if (in&1<<x) {
			subs[subs_idx].module_id = x-1;
			subs[subs_idx++].has_module_id = true;
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
	
#ifdef DEBUG_PRINT
	printf("MDR_version: %f\n", MDR_response.MDR_version);
	printf("module id: %d\n", MDR_response.module_id);
	printf("entity id: %d\n", MDR_response.entity_id);
	printf("module class: %d\n", MDR_response.module_class);
#endif	
	pb_ostream_t output = pb_ostream_from_mp_stream(stream);
	MDR_response.subscriptions.funcs.encode=encode_subscription_callback;
	if(!pb_encode(&output, s2m_MDR_response_fields, &MDR_response)){
	    mp_raise_msg(&mp_type_ValueError, errmsg_encode_error);
	}
	return stream;
    }
    case S2M_DATA:
    {	
	mp_obj_list_t *list = MP_OBJ_TO_PTR(dict);
	/* _datapoint datapoints[16]; /\*< Limit to 16 datapoints for now *\/ */
	/* printf("len: %ld\n", list->len); */
	num_datapoints = list->len;
	for (int x=0; x < list->len; x++) {
	    mp_obj_dict_t *nested_dict = MP_OBJ_TO_PTR(list->items[x]);
	    mp_map_elem_t *elem = NULL;
	    size_t cur = 0;
	    while ((elem = dict_iter_next(nested_dict, &cur)) != NULL) {
		int msg_field_id = get_msg_field_id(msg_id, elem->key);
		switch (msg_field_id) {
		case 1:
		    datapoints[x].entity_id = mp_obj_get_int(elem->value);
		    break;
		case 2:
		    datapoints[x].data = mp_obj_get_float(elem->value);
		    break;
		case 3:
		    datapoints[x].channel_id = mp_obj_get_int(elem->value);
		    datapoints[x].has_channel_id = true;
		    break;
		case 4:
		    datapoints[x].unit_id = mp_obj_get_int(elem->value);
		    datapoints[x].has_unit_id = true;
		    break;
		default:
		    mp_raise_msg(&mp_type_ValueError, errmsg_invalid_field);
		}
	    }
	}
	s2m_data data_message = s2m_data_init_zero;
	data_message.datapoints.funcs.encode=encode_datapoint_callback;
	pb_ostream_t output = pb_ostream_from_mp_stream(stream);
	if (!pb_encode(&output, s2m_data_fields, &data_message)){
	    mp_raise_msg(&mp_type_ValueError, errmsg_encode_error);
	}
	return stream;
	break;
    }
    case S2M_DOC:
    {
	__asm__("nop");
	mp_obj_dict_t *self = MP_OBJ_TO_PTR(dict);
	mp_map_elem_t *elem = NULL;
	size_t cur = 0;
	s2m_DOC doc = s2m_DOC_init_zero;
	while ((elem = dict_iter_next(self, &cur)) != NULL) {
	    int msg_field_id = get_msg_field_id(msg_id, elem->key);
	    switch (msg_field_id) {
	    case 1: /* DOC_code */
		doc.DOC_code = mp_obj_get_int(elem->value);
		break;
	    case 2: /* tx_length */
		doc.tx_length = mp_obj_get_int(elem->value);
		break;
	    default:
		mp_raise_msg(&mp_type_ValueError, errmsg_invalid_field);
		break;
	    }
	}
	pb_ostream_t doc_ostream = pb_ostream_from_mp_stream(stream);
	if (!pb_encode(&doc_ostream, s2m_DOC_fields, &doc)) {
	    mp_raise_msg(&mp_type_ValueError, errmsg_encode_error);
	}
	return stream;
	break;
    }
    case COMMAND:
    {
	__asm__("nop");
	mp_obj_dict_t *self = MP_OBJ_TO_PTR(dict);
	mp_map_elem_t *elem = NULL;
	size_t cur = 0;
	
	command cmd = command_init_zero;
	while ((elem = dict_iter_next(self, &cur)) != NULL) {
	    int msg_field_id = get_msg_field_id(msg_id, elem->key);
	    switch (msg_field_id) {
	    case 1: /* source_module_id */
		cmd.source_module_id = mp_obj_get_int(elem->value);
		break;
	    case 2: /* dest_module_id */
		cmd.dest_module_id = mp_obj_get_int(elem->value);
		break;
	    case 3: /* cmd_str */
		cmd.cmd_str.funcs.encode = encode_cmd_string_callback;
		cmd.cmd_str.arg = (void*)mp_obj_str_get_str(elem->value);
		break;
	    case 4: /* cmd_bytes */
		mp_raise_msg(&mp_type_ValueError, errmsg_unsupported);
		break;
	    default:
		mp_raise_msg(&mp_type_ValueError, errmsg_invalid_field);
		break;
	    }
	}
	pb_ostream_t cmd_ostream = pb_ostream_from_mp_stream(stream);
	if (!pb_encode(&cmd_ostream, command_fields, &cmd)) {
	    mp_raise_msg(&mp_type_ValueError, errmsg_encode_error);
	}
	return stream;
	break;
    }
    default:
	mp_raise_msg(&mp_type_ValueError, errmsg_invalid_msg);	
    }
    
    return mp_obj_new_int(msg_id);
}

STATIC mp_obj_t protobuf_decode(mp_obj_t msg_str, mp_obj_t stream) {
    mp_obj_t dict = mp_obj_new_dict(0);

    int msg_id = get_msg_id(msg_str);
    if (msg_id == 0) {
	mp_raise_msg(&mp_type_ValueError, errmsg_invalid_msg);
    }

    switch (msg_id) {
    case M2S_SOR:
    {
	__asm__("nop");
	char sor_code_key[] = "SOR_code";
	pb_istream_t istream = pb_istream_from_mp_stream(stream);
	m2s_SOR sor_message = m2s_SOR_init_zero;
	pb_decode(&istream, m2s_SOR_fields, &sor_message);
	/* if (!pb_decode(&istream, m2s_SOR_fields, &sor_message)) { */
	/*     mp_raise_msg(&mp_type_ValueError, errmsg_decode_error); */
	/* } */
	mp_obj_t sor_code = mp_obj_new_int(sor_message.SOR_code);
	mp_obj_t sor_key  = mp_obj_new_str(sor_code_key, sizeof(sor_code_key)-1);
	mp_obj_t out_dict = mp_obj_dict_store(dict, sor_key, sor_code);
	return out_dict;
	break;
    }
    case COMMAND:
    {
	__asm__("nop");
	mp_obj_stringio_t *self = MP_OBJ_TO_PTR(stream);
	command cmd = command_init_zero;
	cmd.cmd_str.funcs.decode=decode_cmd_string_callback;	
       
	char *cmd_buf = self->vstr->buf;
	pb_istream_t cmd_istream = pb_istream_from_buffer((unsigned char*)cmd_buf, self->vstr->len);
	str_ptr=0;

	if (pb_decode(&cmd_istream, command_fields, &cmd) != true) {
	    printf("ERROR: %s\n", cmd_istream.errmsg);
	    mp_raise_msg(&mp_type_ValueError, errmsg_decode_error);
	}

	mp_obj_t source_module_id = mp_obj_new_int(cmd.source_module_id);
	mp_obj_t *source_module_id_key = mp_obj_new_str("source_module_id",
							strlen("source_module_id"));

	mp_obj_t dest_module_id = mp_obj_new_int(cmd.dest_module_id);
	mp_obj_t *dest_module_id_key = mp_obj_new_str("dest_module_id", strlen("dest_module_id"));
	
	mp_obj_t *cmd_str = mp_obj_new_str((char*)str_buf, strlen((char*)str_buf));
	mp_obj_t *cmd_str_key = mp_obj_new_str("cmd_str", strlen("cmd_str"));
	
	dict=mp_obj_dict_store(dict, source_module_id_key, source_module_id);
	dict=mp_obj_dict_store(dict, dest_module_id_key, dest_module_id);
	dict=mp_obj_dict_store(dict, cmd_str_key, cmd_str);
	return dict;
    }
    default:
	mp_raise_msg(&mp_type_ValueError, errmsg_invalid_msg);
    }
    return dict;
}

static bool write_callback(pb_ostream_t *stream, const uint8_t *buf, size_t count) {
    int errcode;    
    mp_stream_rw(stream->state, (void*)buf, (mp_uint_t) count, &errcode, MP_STREAM_RW_WRITE);
    if (errcode > 0) {
	return false;
    }
    return true;
}

static bool read_callback(pb_istream_t *stream, pb_byte_t *buf, size_t count) {
    int errcode;
    mp_stream_rw(stream->state, (void*)buf, (mp_uint_t) count, &errcode, MP_STREAM_RW_READ);
    if (errcode > 0) {
	return false;
    }
    return true;
}

pb_ostream_t pb_ostream_from_mp_stream(mp_obj_t stream) {
    pb_ostream_t pb_stream = {&write_callback, (void*)stream, 100, 0};
    return pb_stream;
}

pb_istream_t pb_istream_from_mp_stream(mp_obj_t stream) {
    pb_istream_t pb_stream = {&read_callback, (void*)stream, 10};
    return pb_stream;
}

bool encode_subscription_callback(pb_ostream_t *ostream, const pb_field_t *field, void * const *arg)
{
    if(ostream!=NULL && field->tag == s2m_MDR_response_subscriptions_tag) {
	for (int x=0; x<subs_idx_max; x++) {
	    _subscriptions loc_subs = _subscriptions_init_zero;
	    if (subs[x].has_module_id) {
		loc_subs.has_module_id = true;
		loc_subs.module_id = subs[x].module_id;
#ifdef DEBUG_PRINT
		printf("loc_subs module id: %d\n", subs[x].module_id);
#endif
	    }
	    if (subs[x].has_i2c_address) {
		loc_subs.has_i2c_address = true;
		loc_subs.i2c_address = subs[x].i2c_address;
#ifdef DEBUG_PRINT
		printf("loc_subs i2c_addr: %d\n", subs[x].i2c_address);
#endif
	    }
	    if (subs[x].has_entity_id) {
		loc_subs.has_entity_id = true;
		loc_subs.entity_id = subs[x].entity_id;
	    }
	    /* Module class not supported yet */
	    loc_subs.has_module_class=false;	    

	    if(!pb_encode_tag_for_field(ostream, field)){
#ifdef DEBUG_PRINT
		printf("Encode ERR1\n");
#endif
		return false;
	    }
	    if(!pb_encode_submessage(ostream, _subscriptions_fields, &loc_subs)){
#ifdef DEBUG_PRINT
		printf("Encode ERR2\n");
#endif
		return false;
	    }
	}
    }
    else{
	return false;
    }
    return true;
}

bool encode_datapoint_callback(pb_ostream_t *ostream, const pb_field_t *field, void * const *args)
{
    /* _datapoint *datapoints = *args; */
    if (ostream != NULL && field->tag == s2m_data_datapoints_tag) {
	for (int x=0; x < num_datapoints; x++) {
	    /* printf("data: %f, entity: %d\n", datapoints[x].data, datapoints[x].entity_id); */
	    _datapoint datapoint = _datapoint_init_zero;
	    datapoint.entity_id = datapoints[x].entity_id;
	    datapoint.data      = datapoints[x].data;
	    if (datapoints[x].has_unit_id) {
		datapoint.unit_id = datapoints[x].unit_id;
		datapoint.has_unit_id = true;
	    }
	    if (datapoints[x].has_channel_id) {
		datapoint.channel_id = datapoints[x].channel_id;
		datapoint.has_channel_id = true;
	    }
	    if (!pb_encode_tag_for_field(ostream, field))
		return false;
	    if (!pb_encode_submessage(ostream, _datapoint_fields, &datapoint))
		return false;
	}
    }
    else
	return false;
    return true;
}

bool encode_cmd_string_callback(pb_ostream_t *ostream, const pb_field_t *field, void * const *arg)
{   
    if (ostream != NULL && field->tag == command_cmd_str_tag) {
	if (!pb_encode_tag_for_field(ostream, field))
	    return false;
	return pb_encode_string(ostream, (const unsigned char*)arg[0], strlen((char*)arg[0]));
    }
    return true;
}

bool decode_cmd_string_callback(pb_istream_t *istream, const pb_field_t *field, void **args)
{
    while (istream->bytes_left) {
	if (!pb_read(istream, &str_buf[str_ptr++], 1))
	    return false;
    }
    return true;
}

int get_msg_id(mp_obj_t msg)
{    
    const char m2sMDR_req[] = "m2s_MDR_request",
	s2mMDR_req_ACK[] = "s2m_MDR_req_ACK",
	m2sMDR_res_CTS[] = "m2s_MDR_res_CTS",
	s2mMDR_response[] = "s2m_MDR_response",
	m2sSOR[] = "m2s_SOR",
	s2m_data[] = "s2m_data",
	s2m_DOC[] = "s2m_DOC",
	command[] = "command";
    
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
    else if (strcmp(msg_buf, m2sSOR) == 0)
	id = M2S_SOR;
    else if (strcmp(msg_buf, s2m_data) == 0)
	id = S2M_DATA;
    else if (strcmp(msg_buf, s2m_DOC) == 0)
	id = S2M_DOC;
    else if (strcmp(msg_buf, command) == 0)
	id = COMMAND;
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
    	msg4_subs_i2c_addrs[] = "subs_i2c_addrs",
	msg7_entity_id[] = "entity_id",
	msg7_data[] = "data",
	msg7_unit_id[] = "unit_id",
	msg7_channel_id[] = "channel_id",
	msg6_doc_code[] = "doc_code",
	msg6_tx_length[] = "tx_length",
	msg8_source_module_id[] = "source_module_id",
	msg8_dest_module_id[] = "dest_module_id",
	msg8_cmd_str[] = "cmd_str",
	msg8_cmd_bytes[] = "cmd_bytes";

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
    case S2M_DATA:
	if (strcmp(msg7_entity_id, msg_buf) == 0)
	    id = 1;
	else if (strcmp(msg7_data, msg_buf) == 0)
	    id = 2;
	else if (strcmp(msg7_channel_id, msg_buf) == 0)
	    id = 3;
	else if (strcmp(msg7_unit_id, msg_buf) == 0)
	    id = 4;
	break;
    case S2M_DOC:
	if (strcmp(msg6_doc_code, msg_buf) == 0)
	    id = 1;
	else if (strcmp(msg6_tx_length, msg_buf) == 0)
	    id = 2;
	break;
    case COMMAND:
	if (strcmp(msg8_source_module_id, msg_buf) == 0)
	    id = 1;
	else if (strcmp(msg8_dest_module_id, msg_buf) == 0)
	    id = 2;
	else if (strcmp(msg8_cmd_str, msg_buf) == 0)
	    id = 3;
	else if (strcmp(msg8_cmd_bytes, msg_buf) == 0)
	    id = 4;
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
STATIC MP_DEFINE_CONST_FUN_OBJ_2(protobuf_decode_obj, protobuf_decode);
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pb_enc_obj, pb_enc);


STATIC const mp_rom_map_elem_t protobuf_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_protobuf) },
    { MP_ROM_QSTR(MP_QSTR_encode), MP_ROM_PTR(&protobuf_encode_obj) },
    { MP_ROM_QSTR(MP_QSTR_decode), MP_ROM_PTR(&protobuf_decode_obj) },
    { MP_ROM_QSTR(MP_QSTR_pb_enc), MP_ROM_PTR(&pb_enc_obj) },
};


STATIC MP_DEFINE_CONST_DICT(protobuf_module_globals, protobuf_globals_table);
const mp_obj_module_t protobuf_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&protobuf_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_protobuf, protobuf_user_cmodule, MODULE_PROTOBUF_ENABLED);

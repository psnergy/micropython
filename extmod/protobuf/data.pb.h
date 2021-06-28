/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.2-dev */

#ifndef PB_PROTO_DATA_PB_H_INCLUDED
#define PB_PROTO_DATA_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Struct definitions */
typedef struct _s2m_data {
    pb_callback_t datapoints;
} s2m_data;

typedef struct __datapoint {
    uint32_t entity_id;
    double data;
    bool has_channel_id;
    uint32_t channel_id;
    bool has_unit_id;
    uint32_t unit_id;
    bool has_timestamp;
    int32_t timestamp;
} _datapoint;

typedef struct _command {
    uint32_t source_module_id;
    uint32_t dest_module_id;
    pb_callback_t cmd_bytes;
    pb_callback_t cmd_str;
} command;

typedef struct _m2s_CTS {
    uint32_t timeout;
} m2s_CTS;

typedef struct _m2s_SOR {
    uint32_t SOR_code;
    bool has_rx_length;
    uint32_t rx_length;
} m2s_SOR;

typedef struct _s2m_DOC {
    uint32_t DOC_code;
    uint32_t tx_length;
} s2m_DOC;


/* Initializer values for message structs */
#define m2s_SOR_init_default                     {1u, false, 0}
#define s2m_DOC_init_default                     {1u, 0u}
#define m2s_CTS_init_default                     {0}
#define command_init_default                     {0, 0, {{NULL}, NULL}, {{NULL}, NULL}}
#define _datapoint_init_default                  {0u, 0, false, 1u, false, 1u, false, 1}
#define s2m_data_init_default                    {{{NULL}, NULL}}
#define m2s_SOR_init_zero                        {0, false, 0}
#define s2m_DOC_init_zero                        {0, 0}
#define m2s_CTS_init_zero                        {0}
#define command_init_zero                        {0, 0, {{NULL}, NULL}, {{NULL}, NULL}}
#define _datapoint_init_zero                     {0, 0, false, 0, false, 0, false, 0}
#define s2m_data_init_zero                       {{{NULL}, NULL}}

/* Field tags (for use in manual encoding/decoding) */
#define s2m_data_datapoints_tag                  1
#define _datapoint_entity_id_tag                 1
#define _datapoint_data_tag                      2
#define _datapoint_channel_id_tag                3
#define _datapoint_unit_id_tag                   4
#define _datapoint_timestamp_tag                 5
#define command_source_module_id_tag             1
#define command_dest_module_id_tag               2
#define command_cmd_bytes_tag                    3
#define command_cmd_str_tag                      4
#define m2s_CTS_timeout_tag                      1
#define m2s_SOR_SOR_code_tag                     1
#define m2s_SOR_rx_length_tag                    2
#define s2m_DOC_DOC_code_tag                     1
#define s2m_DOC_tx_length_tag                    2

/* Struct field encoding specification for nanopb */
#define m2s_SOR_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UINT32,   SOR_code,          1) \
X(a, STATIC,   OPTIONAL, UINT32,   rx_length,         2)
#define m2s_SOR_CALLBACK NULL
#define m2s_SOR_DEFAULT (const pb_byte_t*)"\x08\x01\x00"

#define s2m_DOC_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UINT32,   DOC_code,          1) \
X(a, STATIC,   REQUIRED, UINT32,   tx_length,         2)
#define s2m_DOC_CALLBACK NULL
#define s2m_DOC_DEFAULT (const pb_byte_t*)"\x08\x01\x10\x00\x00"

#define m2s_CTS_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UINT32,   timeout,           1)
#define m2s_CTS_CALLBACK NULL
#define m2s_CTS_DEFAULT NULL

#define command_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UINT32,   source_module_id,   1) \
X(a, STATIC,   REQUIRED, UINT32,   dest_module_id,    2) \
X(a, CALLBACK, OPTIONAL, BYTES,    cmd_bytes,         3) \
X(a, CALLBACK, OPTIONAL, STRING,   cmd_str,           4)
#define command_CALLBACK pb_default_field_callback
#define command_DEFAULT NULL

#define _datapoint_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UINT32,   entity_id,         1) \
X(a, STATIC,   REQUIRED, DOUBLE,   data,              2) \
X(a, STATIC,   OPTIONAL, UINT32,   channel_id,        3) \
X(a, STATIC,   OPTIONAL, UINT32,   unit_id,           4) \
X(a, STATIC,   OPTIONAL, INT32,    timestamp,         5)
#define _datapoint_CALLBACK NULL
#define _datapoint_DEFAULT (const pb_byte_t*)"\x08\x00\x11\x00\x00\x00\x00\x00\x00\x00\x00\x18\x01\x20\x01\x28\x01\x00"

#define s2m_data_FIELDLIST(X, a) \
X(a, CALLBACK, REPEATED, MESSAGE,  datapoints,        1)
#define s2m_data_CALLBACK pb_default_field_callback
#define s2m_data_DEFAULT NULL
#define s2m_data_datapoints_MSGTYPE _datapoint

extern const pb_msgdesc_t m2s_SOR_msg;
extern const pb_msgdesc_t s2m_DOC_msg;
extern const pb_msgdesc_t m2s_CTS_msg;
extern const pb_msgdesc_t command_msg;
extern const pb_msgdesc_t _datapoint_msg;
extern const pb_msgdesc_t s2m_data_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define m2s_SOR_fields &m2s_SOR_msg
#define s2m_DOC_fields &s2m_DOC_msg
#define m2s_CTS_fields &m2s_CTS_msg
#define command_fields &command_msg
#define _datapoint_fields &_datapoint_msg
#define s2m_data_fields &s2m_data_msg

/* Maximum encoded size of messages (where known) */
#define m2s_SOR_size                             12
#define s2m_DOC_size                             12
#define m2s_CTS_size                             6
/* command_size depends on runtime parameters */
#define _datapoint_size                          38
/* s2m_data_size depends on runtime parameters */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
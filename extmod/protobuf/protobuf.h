

typedef enum {
    /* Handshake messages  */
    M2S_MDR_REQUEST = 1,
    S2M_MDR_REQ_ACK = 2,
    M2S_MDR_RES_CTS = 3,
    S2M_MDR_RESPONSE = 4,

    /* Dataflow messages */
    M2S_SOR  = 5,
    S2M_DOC  = 6,
    S2M_DATA = 7
} msg_id_t;

#include <pb.h>
#include <pb_encode.h>
#include "py/obj.h"
#include "py/stream.h"

static bool write_callback(pb_ostream_t *stream, const uint8_t *buf, size_t count) {
    int errcode;    
    mp_stream_rw(stream->state, (void*)buf, (mp_uint_t) count, &errcode, MP_STREAM_RW_WRITE);
    if (errcode > 0) {
	return false;
    }
    return true;
}

pb_ostream_t pb_ostream_from_mp_stream(mp_obj_t stream) {
    pb_ostream_t stream = {&write_callback, (void*)stream, 100, 0};
    return stream;
}

PROTOBUF_MOD_DIR := $(USERMOD_DIR)

# Add all C files to SRC_USERMOD.
SRC_USERMOD += $(PROTOBUF_MOD_DIR)/protobuf.c
SRC_USERMOD += $(PROTOBUF_MOD_DIR)/pb_common.c
SRC_USERMOD += $(PROTOBUF_MOD_DIR)/pb_encode.c
SRC_USERMOD += $(PROTOBUF_MOD_DIR)/handshake.pb.c

# We can add our module folder to include paths if needed
# This is not actually needed in this example.
CFLAGS_USERMOD += -I$(PROTOBUF_MOD_DIR)

all: \
    $(BUILT_PRODUCTS_DIR)/pyproto/webrtc/audio_processing/debug_pb2.py

$(BUILT_PRODUCTS_DIR)/pyproto/webrtc/audio_processing/debug_pb2.py \
    $(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing/debug.pb.cc \
    $(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing/debug.pb.h \
    : \
    audio_processing/debug.proto \
    $(BUILT_PRODUCTS_DIR)/protoc
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/pyproto/webrtc/audio_processing" "$(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing"
	@echo note: "Generating C++ and Python code from audio_processing/debug.proto"
	"$(BUILT_PRODUCTS_DIR)/protoc" "--proto_path=audio_processing" "audio_processing/debug.proto" "--cpp_out=$(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing" "--python_out=$(BUILT_PRODUCTS_DIR)/pyproto/webrtc/audio_processing"

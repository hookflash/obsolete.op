all: \
    $(BUILT_PRODUCTS_DIR)/pyproto/webrtc/audio_processing/unittest_pb2.py

$(BUILT_PRODUCTS_DIR)/pyproto/webrtc/audio_processing/unittest_pb2.py \
    $(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing/unittest.pb.cc \
    $(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing/unittest.pb.h \
    : \
    audio_processing/test/unittest.proto \
    $(BUILT_PRODUCTS_DIR)/protoc
	@mkdir -p "$(BUILT_PRODUCTS_DIR)/pyproto/webrtc/audio_processing" "$(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing"
	@echo note: "Generating C++ and Python code from audio_processing/test/unittest.proto"
	"$(BUILT_PRODUCTS_DIR)/protoc" "--proto_path=audio_processing/test" "audio_processing/test/unittest.proto" "--cpp_out=$(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing" "--python_out=$(BUILT_PRODUCTS_DIR)/pyproto/webrtc/audio_processing"

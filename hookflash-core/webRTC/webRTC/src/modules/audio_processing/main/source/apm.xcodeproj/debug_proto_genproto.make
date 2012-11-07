all: \
    $(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing/debug.pb.cc

$(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing/debug.pb.cc \
    $(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing/debug.pb.h \
    : \
    ./debug.proto \
    $(BUILT_PRODUCTS_DIR)/protoc
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing"
	@echo note: "Generating C++ code from ./debug.proto"
	"$(BUILT_PRODUCTS_DIR)/protoc" "--proto_path=." "./debug.proto" "--cpp_out=$(SHARED_INTERMEDIATE_DIR)/protoc_out/webrtc/audio_processing"

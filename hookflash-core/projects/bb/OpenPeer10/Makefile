# Makefile for the Blackberry OpenPeer library

HOOKFLASH_ROOT  = /Users/lawrence.gunn/Documents/op

HOOKFLASH_BOOST = $(HOOKFLASH_ROOT)/hookflash-libs/bbBoost/bin.v2/libs
HOOKFLASH_BB_LIBS = $(HOOKFLASH_ROOT)/hookflash-core/projects/bb
BOOST_LIB_TYPE = build/qcc/debug/architecture-arm/link-static/target-os-qnxnto/threading-multi

ifeq ($(MAKECMDGOALS),SimulationDebug)
	BB_PLATFORM = Simulation-Debug
else
	BB_PLATFORM = Device-Debug
endif

#			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/common_video/$(BB_PLATFORM)/libcommon_video.a \

DeviceDebug:
	qcc -o $(BB_PLATFORM)/libOpenPeer10.so $(BB_PLATFORM)/public/opVersion.o \
		-lcurl -lsocket -lbps -lm -lc -lcpp -lstdc++ -ljpeg -lGLESv2 \
		-Wl,--whole-archive \
			-l$(HOOKFLASH_BOOST)/date_time/$(BOOST_LIB_TYPE)/libboost_date_time.a \
			-l$(HOOKFLASH_BOOST)/thread/$(BOOST_LIB_TYPE)/libboost_thread.a \
			-l$(HOOKFLASH_BOOST)/system/$(BOOST_LIB_TYPE)/libboost_system.a \
			-l$(HOOKFLASH_BOOST)/regex/$(BOOST_LIB_TYPE)/libboost_regex.a \
			-l$(HOOKFLASH_BB_LIBS)/bbzsLib/$(BB_PLATFORM)/libbbzsLib.a \
			-l$(HOOKFLASH_BB_LIBS)/bbcryptopp/$(BB_PLATFORM)/libbbcryptopp.a \
			-l$(HOOKFLASH_BB_LIBS)/bbudns/$(BB_PLATFORM)/libbbudns.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/system_wrappers/$(BB_PLATFORM)/libsystem_wrappers.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/common_audio/$(BB_PLATFORM)/libcommon_audio.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/jpeg/$(BB_PLATFORM)/libjpeg.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/yuv/$(BB_PLATFORM)/libyuv.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/common_video/$(BB_PLATFORM)/libcommon_video.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/voice_engine/$(BB_PLATFORM)/libvoice_engine.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/video_engine/$(BB_PLATFORM)/libvideo_engine.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/vpx/$(BB_PLATFORM)/libvpx.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/modules/$(BB_PLATFORM)/libmodules.a \
			-l$(HOOKFLASH_BB_LIBS)/bbhfservices/$(BB_PLATFORM)/libbbhfservices.a \
			-l$(HOOKFLASH_BB_LIBS)/bbhfstack/$(BB_PLATFORM)/libbbhfstack.a \
 			-l$(HOOKFLASH_BB_LIBS)/bbhfcore/$(BB_PLATFORM)/libbbhfcore.a \
		-Wl,--no-whole-archive \
		-V4.6.3,gcc_ntoarmv7le_cpp -w1 -lang-c++ -shared -Wl,-E -g -Wl,-z,relro -Wl,-z,now -L/Applications/bbndk/target_10_1_0_338/qnx6/../target-override/armle-v7/lib -L/Applications/bbndk/target_10_1_0_338/qnx6/../target-override/armle-v7/usr/lib -Wl,-z,defs

SimulationDebug:
	mkdir $(BB_PLATFORM)
	qcc -o $(BB_PLATFORM)/libOpenPeer10.so $(BB_PLATFORM)/public/opVersion.o \
		-lcurl -lsocket -lbps -lm -lc -lcpp -lstdc++ -ljpeg -lGLESv2 \
		-Wl,--whole-archive \
			-l$(HOOKFLASH_BOOST)/date_time/$(BOOST_LIB_TYPE)/libboost_date_time.a \
			-l$(HOOKFLASH_BOOST)/thread/$(BOOST_LIB_TYPE)/libboost_thread.a \
			-l$(HOOKFLASH_BOOST)/system/$(BOOST_LIB_TYPE)/libboost_system.a \
			-l$(HOOKFLASH_BOOST)/regex/$(BOOST_LIB_TYPE)/libboost_regex.a \
			-l$(HOOKFLASH_BB_LIBS)/bbzsLib/$(BB_PLATFORM)/libbbzsLib.a \
			-l$(HOOKFLASH_BB_LIBS)/bbcryptopp/$(BB_PLATFORM)/libbbcryptopp.a \
			-l$(HOOKFLASH_BB_LIBS)/bbudns/$(BB_PLATFORM)/libbbudns.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/system_wrappers/$(BB_PLATFORM)/libsystem_wrappers.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/common_audio/$(BB_PLATFORM)/libcommon_audio.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/jpeg/$(BB_PLATFORM)/libjpeg.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/yuv/$(BB_PLATFORM)/libyuv.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/common_video/$(BB_PLATFORM)/libcommon_video.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/voice_engine/$(BB_PLATFORM)/libvoice_engine.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/video_engine/$(BB_PLATFORM)/libvideo_engine.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/vpx/$(BB_PLATFORM)/libvpx.a \
			-l$(HOOKFLASH_BB_LIBS)/bbWebRTCLibs/modules/$(BB_PLATFORM)/libmodules.a \
			-l$(HOOKFLASH_BB_LIBS)/bbhfservices/$(BB_PLATFORM)/libbbhfservices.a \
			-l$(HOOKFLASH_BB_LIBS)/bbhfstack/$(BB_PLATFORM)/libbbhfstack.a \
 			-l$(HOOKFLASH_BB_LIBS)/bbhfcore/$(BB_PLATFORM)/libbbhfcore.a \
		-Wl,--no-whole-archive \
		-V4.6.3,gcc_ntox86_cpp -w1 -lang-c++ -shared -g -Wl,-z,relro -Wl,-z,now -L/Applications/bbndk/target_10_1_0_338/qnx6/../target-override/x86/lib -L/Applications/bbndk/target_10_1_0_338/qnx6/../target-override/x86/usr/lib

	
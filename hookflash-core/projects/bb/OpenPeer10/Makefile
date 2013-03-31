# Makefile for the Blackberry OpenPeer library

HOOKFLASH_ROOT  = /Users/lawrence.gunn/Documents/op
HOOKFLASH_BOOST = $(HOOKFLASH_ROOT)/hookflash-libs/bbBoost/bin.v2/libs
HOOKFLASH_BB_LIBS = $(HOOKFLASH_ROOT)/hookflash-core/projects/bb
BOOST_LIB_TYPE = build/qcc/debug/architecture-arm/link-static/target-os-qnxnto/threading-multi
BB_PLATFORM = Device-Debug

DeviceDebug:
	qcc -o Device-Debug/libOpenPeer10.so Device-Debug/public/opVersion.o \
		-lcurl -lsocket -lbps -lm -lc -lcpp -lstdc++ \
		-Wl,--whole-archive \
			-l$(HOOKFLASH_BOOST)/date_time/$(BOOST_LIB_TYPE)/libboost_date_time.a \
			-l$(HOOKFLASH_BOOST)/thread/$(BOOST_LIB_TYPE)/libboost_thread.a \
			-l$(HOOKFLASH_BOOST)/system/$(BOOST_LIB_TYPE)/libboost_system.a \
			-l$(HOOKFLASH_BOOST)/regex/$(BOOST_LIB_TYPE)/libboost_regex.a \
			-l$(HOOKFLASH_BB_LIBS)/bbzsLib/$(BB_PLATFORM)/libbbzsLib.a \
			-l$(HOOKFLASH_BB_LIBS)/bbcryptopp/$(BB_PLATFORM)/libbbcryptopp.a \
			-l$(HOOKFLASH_BB_LIBS)/bbudns/$(BB_PLATFORM)/libbbudns.a \
		-Wl,--no-whole-archive \
		-V4.6.3,gcc_ntoarmv7le_cpp -w1 -lang-c++ -shared -Wl,-E -g -Wl,-z,relro -Wl,-z,now -L/Applications/bbndk/target_10_1_0_338/qnx6/../target-override/armle-v7/lib -L/Applications/bbndk/target_10_1_0_338/qnx6/../target-override/armle-v7/usr/lib -Wl,-z,defs


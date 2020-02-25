sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


FILES:= \
	Config.cpp \
	DMXController.cpp \
	LightPacket.cpp \
	Logger.cpp \
	# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


afl-lightpackets_MODULES:=date fmt
afl-lightpackets_SRC:=$(SRC_$(d)) $(dir)/afl-lightpackets.cpp


libfuzzer-lightpackets_MODULES:=date fmt
libfuzzer-lightpackets_SRC:=$(SRC_$(d)) $(dir)/libfuzzer-lightpackets.cpp


serveri_MODULES:=date fmt
serveri_SRC:=$(SRC_$(d)) $(dir)/serveri.cpp


ifeq ($(USE_LIBWEBSOCKETS),y)

serveri_MODULES+=libwebsockets

CFLAGS+=-DUSE_LIBWEBSOCKETS

endif  # USE_LIBWEBSOCKETS

test_MODULES:=date fmt
test_SRC:=$(SRC_$(d)) $(dir)/test.cpp


ifeq ($(LIBFUZZER), y)

PROGRAMS += libfuzzer-lightpackets

else  # LIBFUZZER

PROGRAMS+= \
	afl-lightpackets \
	serveri \
	test \
	# empty line

endif  # LIBFUZZER


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

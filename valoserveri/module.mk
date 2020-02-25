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


serveri_MODULES:=date fmt
serveri_SRC:=$(SRC_$(d)) $(dir)/serveri.cpp


ifeq ($(USE_LIBWEBSOCKETS),y)

serveri_MODULES+=libwebsockets

CFLAGS+=-DUSE_LIBWEBSOCKETS

endif  # USE_LIBWEBSOCKETS

test_MODULES:=date fmt
test_SRC:=$(SRC_$(d)) $(dir)/test.cpp


PROGRAMS+= \
	serveri \
	test \
	# empty line


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

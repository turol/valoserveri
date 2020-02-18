sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


FILES:= \
	Config.cpp \
	DMXController.cpp \
	Logger.cpp \
	# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


serveri_MODULES:=date fmt libwebsockets
serveri_SRC:=$(SRC_$(d)) $(dir)/serveri.cpp


test_MODULES:=date fmt
test_SRC:=$(SRC_$(d)) $(dir)/test.cpp


PROGRAMS+= \
	serveri \
	test \
	# empty line


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

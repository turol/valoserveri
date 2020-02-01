sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


FILES:= \
	# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


serveri_MODULES:=
serveri_SRC:=$(SRC_$(d)) $(dir)/serveri.cpp


test_MODULES:=
test_SRC:=$(SRC_$(d)) $(dir)/test.cpp


PROGRAMS+= \
	serveri \
	test \
	# empty line


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

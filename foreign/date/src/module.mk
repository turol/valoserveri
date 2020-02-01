sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


SUBDIRS:= \
	# empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


FILES:= \
	tz.cpp \
	# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


SRC_date:=$(SRC_$(d))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

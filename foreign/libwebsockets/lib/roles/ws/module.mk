sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


SUBDIRS:= \
	# empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


FILES:= \
	client-parser-ws.c \
	client-ws.c \
	ops-ws.c \
	server-ws.c \
	# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES)) $(foreach directory, $(DIRS), $(SRC_$(directory)))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

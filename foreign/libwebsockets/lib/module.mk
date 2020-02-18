sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


SUBDIRS:= \
	core \
	core-net \
	event-libs \
	misc \
	plat \
	roles \
	# empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


FILES:= \
	# empty line


SRC_libwebsockets:=$(addprefix $(d)/,$(FILES)) $(foreach directory, $(DIRS), $(SRC_$(directory)))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

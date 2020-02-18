sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


SUBDIRS:= \
	lwsac \
	# empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


FILES:= \
	base64-decode.c \
	dir.c \
	diskcache.c \
	lejp.c \
	lws-ring.c \
	lws-struct-lejp.c \
	sha-1.c \
	# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES)) $(foreach directory, $(DIRS), $(SRC_$(directory)))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

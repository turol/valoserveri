sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


SUBDIRS:= \
	# empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


FILES:= \
	adopt.c \
	client.c \
	close.c \
	connect.c \
	dummy-callback.c \
	lws-dsh.c \
	network.c \
	output.c \
	pollfd.c \
	sequencer.c \
	server.c \
	service.c \
	sorted-usec-list.c \
	stats.c \
	vhost.c \
	wsi.c \
	wsi-timeout.c \
	# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES)) $(foreach directory, $(DIRS), $(SRC_$(directory)))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

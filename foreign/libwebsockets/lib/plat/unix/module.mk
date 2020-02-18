sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


SUBDIRS:= \
	# empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


FILES:= \
	unix-caps.c \
	unix-fds.c \
	unix-file.c \
	unix-init.c \
	unix-misc.c \
	unix-pipe.c \
	unix-service.c \
	unix-sockets.c \
	# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES)) $(foreach directory, $(DIRS), $(SRC_$(directory)))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


SUBDIRS:= \
	# empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


FILES:= \
	fops-zip.c \
	lejp-conf.c \
	lws-spa.c \
	parsers.c \
	rewrite.c \
	server.c \
	# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES)) $(foreach directory, $(DIRS), $(SRC_$(directory)))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

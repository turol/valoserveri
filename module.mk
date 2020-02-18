.PHONY: default all bindirs clean cppcheck distclean


.SUFFIXES:

#initialize these
PROGRAMS:=
ALLSRC:=
# directories which might contain object files
# used for both clean and bindirs
ALLDIRS:=

default: all


ifeq ($(ASAN),y)

OPTFLAGS+=-fsanitize=address
LDFLAGS_asan?=-fsanitize=address
LDFLAGS+=$(LDFLAGS_asan)

endif  # ASAN


ifeq ($(TSAN),y)

CFLAGS+=-DPIC
OPTFLAGS+=-fsanitize=thread -fpic
LDFLAGS_tsan?=-fsanitize=thread -pie
LDFLAGS+=$(LDFLAGS_tsan)

endif  # TSAN


ifeq ($(UBSAN),y)

OPTFLAGS+=-fsanitize=undefined -fno-sanitize-recover=all
LDFLAGS+=-fsanitize=undefined -fno-sanitize-recover=all

endif  # UBSAN


ifeq ($(LTO),y)

CFLAGS+=$(LTOCFLAGS)
LDFLAGS+=$(LTOLDFLAGS) $(OPTFLAGS)

endif  # LTO


# date tz
CFLAGS+=-DHAS_REMOTE_API=0 -DUSE_OS_TZDB=1

CFLAGS+=$(OPTFLAGS)

CFLAGS+=-I.
CFLAGS+=-I$(TOPDIR)
CFLAGS+=-isystem$(TOPDIR)/foreign
CFLAGS+=-isystem$(TOPDIR)/foreign/date/include
CFLAGS+=-isystem$(TOPDIR)/foreign/fmt/include
CFLAGS+=-isystem$(TOPDIR)/foreign/libwebsockets/include
CFLAGS+=-isystem$(TOPDIR)/foreign/libwebsockets/lib


# (call directory-module, dirname)
define directory-module

# save old
DIRS_$1:=$$(DIRS)

dir:=$1
include $(TOPDIR)/$1/module.mk

ALLDIRS+=$1

ALLSRC+=$$(SRC_$1)

# restore saved
DIRS:=$$(DIRS_$1)

endef  # directory-module


DIRS:= \
	foreign \
	valoserveri \
	# empty line
$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


TARGETS:=$(foreach PROG,$(PROGRAMS),$(EXEPREFIX)$(PROG)$(EXESUFFIX))

all: $(TARGETS)


# check if a directory needs to be created
# can't use targets with the same name as directory because unfortunate
# interaction with VPATH (targets always exists because source dir)
#  $(call missingdir, progname)
define missingdir

ifneq ($$(shell test -d $1 && echo n),n)
MISSINGDIRS+=$1
endif

endef # missingdir


MISSINGDIRS:=
$(eval $(foreach d, $(ALLDIRS), $(call missingdir,$(d)) ))


# create directories which might contain object files
bindirs:
ifneq ($(MISSINGDIRS),)
	mkdir -p $(MISSINGDIRS)
endif


clean:
	rm -f $(TARGETS) $(foreach dir,$(ALLDIRS),$(dir)/*$(OBJSUFFIX))

distclean: clean
	rm -f $(foreach dir,$(ALLDIRS),$(dir)/*.d)
	-rmdir -p --ignore-fail-on-non-empty $(ALLDIRS)


# rules here

# -MD generates dependencies for system headers
# -MMD does not
# we use -MD because we ship some headers which are included with -isystem
# to avoid generating warnings from them
# but we want them to be dependencies

%$(OBJSUFFIX): %.cpp | bindirs
	$(CXX) -c -MF $*.d -MP -MD $(CXXFLAGS) -o $@ $<

%$(OBJSUFFIX): %.c | bindirs
	$(CC) -c -MF $*.d -MP -MD $(CFLAGS) -o $@ $<

# no warnings in foreign code
foreign/%$(OBJSUFFIX): foreign/%.cpp | bindirs
	$(CXX) -c -MF foreign/$*.d -MP -MD $(CXXFLAGS) -w -o $@ $<

foreign/%$(OBJSUFFIX): foreign/%.cc | bindirs
	$(CXX) -c -MF foreign/$*.d -MP -MD $(CXXFLAGS) -w -o $@ $<

foreign/%$(OBJSUFFIX): foreign/%.c | bindirs
	$(CC) -c -MF foreign/$*.d -MP -MD $(CFLAGS) -w -o $@ $<


# $(call resolve-modules, progname)
define resolve-modules

OLD_MODULES:=$$($1_MODULES)

$1_MODULES:=$$(sort $$($1_MODULES) $$(foreach MODULE, $$($1_MODULES), $$(DEPENDS_$$(MODULE))) )

# recurse if changed
ifneq ($$($1_MODULES),$$(OLD_MODULES))
$$(eval $$(call resolve-modules,$1) )
endif

endef  # resolve-modules


$(eval $(foreach PROGRAM,$(PROGRAMS), $(call resolve-modules,$(PROGRAM)) ) )


# $(call program-target, progname)
define program-target

ALLSRC+=$$(filter %.cpp,$$($1_SRC))
ALLSRC+=$$(filter %.cc,$$($1_SRC))
ALLSRC+=$$(filter %.c,$$($1_SRC))

$1_SRC+=$$(foreach module, $$($1_MODULES), $$(SRC_$$(module)))
$1_OBJ:=$$($1_SRC:.cpp=$(OBJSUFFIX))
$1_OBJ:=$$($1_OBJ:.cc=$(OBJSUFFIX))
$1_OBJ:=$$($1_OBJ:.c=$(OBJSUFFIX))
$(EXEPREFIX)$1$(EXESUFFIX): $$($1_OBJ) | bindirs
	$(CXX) $(LDFLAGS) -o $$@ $$^ $$(foreach module, $$($1_MODULES), $$(LDLIBS_$$(module))) $$($1_LIBS) $(LDLIBS)

endef  # program-target


$(eval $(foreach PROGRAM,$(PROGRAMS), $(call program-target,$(PROGRAM)) ) )


export CXX
export CXXFLAGS
export TOPDIR


compile_commands.json: $(ALLSRC)
	@echo '[' > $@
	@$(TOPDIR)/compile_commands.sh $(filter %.cpp,$(ALLSRC)) >> $@
	@echo ']' >> $@


JOBS?=1


cppcheck:
	cppcheck -j $(JOBS) $(foreach directory,$(INCLUDEDIRS),-I $(directory)) -i $(TOPDIR)/foreign --enable=all $(TOPDIR) 2> cppcheck.log


-include $(foreach FILE,$(ALLSRC),$(patsubst %.c,%.d,$(patsubst %.cpp,%.d,$(patsubst %.cc,%.d,$(FILE)))))

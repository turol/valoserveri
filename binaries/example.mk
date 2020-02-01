# example of local configuration
# copy to local.mk


# location of source
TOPDIR:=..


LTO:=n
ASAN:=n
TSAN:=n
UBSAN:=n


# compiler options etc
CXX:=g++
CC:=gcc
CFLAGS:=-g -Wall -Wextra -Wshadow
CFLAGS+=-Wno-unused-local-typedefs

OPTFLAGS:=-Os
OPTFLAGS+=-march=native
# OPTFLAGS+=-ffunction-sections -fdata-sections


# lazy assignment because CFLAGS is changed later
CXXFLAGS=$(CFLAGS)
CXXFLAGS+=-std=c++14


LDFLAGS:=-g
#LDFLAGS+=-Wl,--gc-sections
# you can enable this if you're using gold linker
#LFDLAGS+=-Wl,--icf=all
LDLIBS:=-lpthread

LTOCFLAGS:=-flto -fuse-linker-plugin -fno-fat-lto-objects
LTOLDFLAGS:=-flto -fuse-linker-plugin


OBJSUFFIX:=.o
EXESUFFIX:=-bin

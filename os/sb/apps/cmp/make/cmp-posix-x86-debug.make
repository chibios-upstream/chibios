PROJECT := cmp
CHIBIOS := ../../../..

CONFDIR  := ./cfg/cmp-posix-x86-debug
BUILDDIR := ../build/posix/cmp-posix-x86-debug
DEPDIR   := ../build/posix/.dep/cmp-posix-x86-debug

SBAPP_CSRC := main.c
SBAPP_TESTS := ./test/cmp.sh

include ../common/make/cmdutil.mk
include ../common/make/app-posix-x86.mk

PROJECT := cp
CHIBIOS := ../../../..

CONFDIR  := ./cfg/cp-posix-x86-debug
BUILDDIR := ../build/posix/cp-posix-x86-debug
DEPDIR   := ../build/posix/.dep/cp-posix-x86-debug

SBAPP_CSRC := main.c
SBAPP_TESTS := ./test/cp.sh

include ../common/make/cmdutil.mk
include ../common/make/app-posix-x86.mk

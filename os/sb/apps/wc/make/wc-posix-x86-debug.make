PROJECT := wc
CHIBIOS := ../../../..

CONFDIR  := ./cfg/wc-posix-x86-debug
BUILDDIR := ../build/posix/wc-posix-x86-debug
DEPDIR   := ../build/posix/.dep/wc-posix-x86-debug

SBAPP_CSRC := main.c
SBAPP_TESTS := ./test/wc.sh

include ../common/make/cmdutil.mk
include ../common/make/app-posix-x86.mk

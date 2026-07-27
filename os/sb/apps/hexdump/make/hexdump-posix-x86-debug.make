PROJECT := hexdump
CHIBIOS := ../../../..

CONFDIR  := ./cfg/hexdump-posix-x86-debug
BUILDDIR := ./build/hexdump-posix-x86-debug
DEPDIR   := ./.dep/hexdump-posix-x86-debug

SBAPP_CSRC := main.c
SBAPP_TESTS := ./test/hexdump.sh

include ../common/make/cmdutil.mk
include ../common/make/app-posix-x86.mk

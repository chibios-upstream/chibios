PROJECT := sleep
CHIBIOS := ../../../..

CONFDIR  := ./cfg/sleep-posix-x86-debug
BUILDDIR := ./build/sleep-posix-x86-debug
DEPDIR   := ./.dep/sleep-posix-x86-debug

SBAPP_CSRC := main.c
SBAPP_UDEFS := -DSBAPP_NATIVE
SBAPP_TESTS := ./test/sleep.sh

include ../common/make/cmdutil.mk
include ../common/make/app-posix-x86.mk

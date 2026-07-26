PROJECT := head
CHIBIOS := ../../../..

CONFDIR  := ./cfg/head-posix-x86-debug
BUILDDIR := ./build/head-posix-x86-debug
DEPDIR   := ./.dep/head-posix-x86-debug

SBAPP_CSRC := main.c
SBAPP_TESTS := ./test/head.sh

include ../common/make/cmdutil.mk
include ../common/make/app-posix-x86.mk

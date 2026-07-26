PROJECT := stat
CHIBIOS := ../../../..

CONFDIR  := ./cfg/stat-posix-x86-debug
BUILDDIR := ../build/posix/stat-posix-x86-debug
DEPDIR   := ../build/posix/.dep/stat-posix-x86-debug

SBAPP_CSRC := main.c
SBAPP_TESTS := ./test/stat.sh

include ../common/make/cmdutil.mk
include ../common/make/app-posix-x86.mk

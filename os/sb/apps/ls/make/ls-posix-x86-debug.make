PROJECT := ls
CHIBIOS := ../../../..

CONFDIR  := ./cfg/ls-posix-x86-debug
BUILDDIR := ../build/posix/ls-posix-x86-debug
DEPDIR   := ../build/posix/.dep/ls-posix-x86-debug

SBAPP_TESTS := ./test/ls.sh

include ../common/make/app-posix-x86.mk

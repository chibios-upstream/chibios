# Relocatable ELF execution support for sandbox command launchers.

SBELFEXECDIR := $(CHIBIOS)/os/sb/apps/common/elfexec

ALLCSRC     += $(SBELFEXECDIR)/elfexec.c
ALLXASMSRC  += $(SBELFEXECDIR)/callhdr.S
ALLINC      += $(SBELFEXECDIR)

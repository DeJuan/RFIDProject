#
#  Makefile to build the Java implementation of the Mercury API
#

#
# Default target
#
default: all

ifdef TM_DIR
BUILD      ?= Release
else
BUILD      ?= .
endif
TM_LEVEL   ?= ../../..
TOP_LEVEL  ?= ../../..
SRCDIR     ?= .
MODULESDIR ?= ${TM_DIR}/modules
PLATFORM   ?= PC
CFLAGS     += -Wall
ifeq ($(PLATFORM), PC)
  PLATFORM_CODE := x86
  PROCESSOR     := x86
else
  PLATFORM_CODE := arm
  PROCESSOR     := arm
endif

NODEPTARGETS = clean
TARGETS :=
OBJECTS :=
CLEANS :=
ALL_OBJS :=
ALL_C_SRCS :=

MKDIR ?= mkdir -p
TEST ?= test

#ifeq ($(PLATFORM_CODE), arm)
#  include $(TOP_LEVEL)/arch/ARM/ixp42x/module.mk
#endif
#include $(TOP_LEVEL)/arch/linux/module.mk

include ./module.mk

DEPS := $(ALL_OBJS:.class=.d)
ifeq ($(OMITDEPS),0)
-include $(DEPS)
endif

# Create the build directory
create_build_dir := $(shell $(TEST) -d $(BUILD) || $(MKDIR) $(BUILD))

#
# main target rules
#
all: $(TARGETS)

clean:
	rm -fr $(CLEANS) $(TARGETS) $(DEPS) $(ALL_OBJS)
	@echo "All generated files removed."

.PHONY: cleanautoparams
cleanautoparams:
	svn revert $(AUTOPARAM_CLEANS)


# For internal use only.  Distribution should already include the following target files.
ifdef ENABLE_INTERNAL_TARGETS
ifneq ($(ENABLE_INTERNAL_TARGETS),0)
-include externals.mk
endif
endif